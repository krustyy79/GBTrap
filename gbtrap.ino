/***************************************************
 * Coded by Krustyy
 * version 1.0 
 * utilizes the Couroutines library and fastLED library
 * https://github.com/renaudbedard/littlebits-arduino/tree/master/Libraries/Coroutines
 * https://github.com/FastLED/FastLED/archive/master.zip
 ***************************************************/

 /**************************************
  Notes for Myself
  Need to add a small LED somewhere to indicate power. This is not controlled by arduino and goes directly to power
  Trap appears to have mostly white light coming out of it with purple and orange flashy stuff I could add a blue LED to the handle to aim backwards during trap suction but thats unnecessary
  after trap activates, led bar, in yellow or orange, lights up from left to right. I should use as many LEDs as I can there.
  Then an additional LED on the far right of the bar, same color lights up. 
  red light flashes and beeps for a while afterwards.
  Light then goes solid and smoke pours out.

  ***********************************************/



#include <Coroutines.h>
#include <Servo.h>
#include <FastLED.h>

// --trap variables
int trapState = 0; 
const int trapIdle = 0;         // Idle
const int trapOpening = 1;      // trigger press waiting for doors to open
const int trapOpenShow1 = 2;    // doors open and audio / lights active
const int trapOpenShow2 = 3;    // capture sequence with added audio / lights
const int trapOpenIdle = 4;     // doors open and audio / lights done
const int trapClosing = 5;      // closing

// --door variables: servo1--  SERVO OPEN AND CLOSED VALUES NEED TO BE ADJUSTED FOR YOUR SERVOS!
Servo servo1;
const int servo1Pin = 9;
const int servo1OpenPosition = 68;
//const int servo1ClosedPosition = 23; tried 0 and it seemed to work
const int servo1ClosedPosition = 10;

// --door variables: servo_2--
Servo servo2;
const int servo2Pin = 10;
const int servo2OpenPosition = 15;
//const int servo2ClosedPosition = 63; tried 86 and it seemed to work
const int servo2ClosedPosition = 76;


// --door variables: state--
unsigned long doorTimer = 0;
unsigned long trapTimer = 0;
bool ghostTrapped = false;

const int doorWaitTime = 300; // If your doors don't have enough time to close, make this longer

// twinkle duration variable
bool twinkleLoop = true;

// for closing the door upon the first button push
bool closeDoor = false;

// triggerState tracks whether or not the trigger is on or off
int triggerState = HIGH;
int fullTriggerState = HIGH;
int triggerPress = false;
int fullTriggerCycle = false;

const unsigned long openTime = 15000;         // How long the trap will remain open before auto closing
const unsigned long trapTime = 8500;          //This is how long the trapping cycle will run after the second button press
const unsigned long twinkleDelay = 750;      // How long after the trap closing sequence runs will the internal lights continue to be on
const unsigned long smokeTime = 3000;         // How long the smoke should run for each smoke cycle

CRGB yellowLEDs[5];           // FastLED LED array
CRGB innerLEDs[6];            // FastLED LED array
CRGB redLED[1];               // FastLED LED array

// -- Coroutine Stuff --
Coroutines<8> coroutines;

void setup() {
  Serial.begin(9600);

  Serial.println("Rebooted");


  
  FastLED.addLeds<WS2812B, 5, GRB>(innerLEDs, 6);     //pin 5 is the interior LEDs
  FastLED.addLeds<WS2812B, 3, GRB>(yellowLEDs, 5);    //pin 3 is the yellow LEDs outside
  FastLED.addLeds<WS2812B, 11, GRB>(redLED, 1);       //pin 11 is the red LED

  // New Pin Configuration
  // 3 is yellow LEDs
  // 5 is inner LEDs
  pinMode(7 , OUTPUT);                                // smoke
  pinMode(8 , OUTPUT);                                // 5v lights
  pinMode(9 , OUTPUT);                                // Servo1
  pinMode(10 , OUTPUT);                               // Servo2
  // 11 is red LED
  pinMode(12 , INPUT_PULLUP);                         // full trap trigger
  pinMode(14 , OUTPUT);                               // audio 1 trap open
  pinMode(15 , OUTPUT);                               // audio 2 trap close
  pinMode(16 , INPUT_PULLUP);                         // trap trigger
  



  // Shut off lights (and turn on red light)
  yellowLEDs[0] = CRGB::Black;
  yellowLEDs[1] = CRGB::Black;
  yellowLEDs[2] = CRGB::Black;
  yellowLEDs[3] = CRGB::Black;
  yellowLEDs[4] = CRGB::Black;
  redLED[0]     = CRGB::Red;
  innerLEDs[0]  = CRGB::Black;
  innerLEDs[1]  = CRGB::Black;   
  innerLEDs[2]  = CRGB::Black;
  innerLEDs[3]  = CRGB::Black;
  innerLEDs[4]  = CRGB::Black;
  innerLEDs[5]  = CRGB::Black;
  FastLED.show();

  //open the doors to allow access to the system
  closeDoor = false;
  AttachServos();
  MoveServos(servo1OpenPosition, servo2OpenPosition);
  delay(doorWaitTime);
  DetachServos();

  // reset variables
  doorTimer = 0;
  trapTimer = 0;
  ghostTrapped = false;

  triggerState = HIGH;
  fullTriggerState = HIGH;
  triggerPress = false;
  fullTriggerCycle = false;

}

void loop() {
  
  // First, read the state of the trap triggers
  triggerState = digitalRead(16);
  fullTriggerState = digitalRead(12);
  // Update the LEDs and coroutines
  coroutines.update();

  // If this is the first button press, close the doors and prepare the trap
  if ((closeDoor == false && triggerPress == false && triggerState == LOW) || (closeDoor == false && fullTriggerState == LOW && triggerPress == false) ){
    closeDoor = true;
    AttachServos();
    MoveServos(servo1ClosedPosition, servo2ClosedPosition);
    delay(doorWaitTime);
    DetachServos();
    redLED[0]     = CRGB::Black;
    FastLED.show();
    triggerPress = true;
  }
  // This block detects if a button has been newly pressed (button down) and the trap is in a state to accept trigger presses
  // If we are already in a full trigger cycle we run through this loop as well
  else if ( (triggerState == LOW && triggerPress == false && trapState != trapOpening && trapState != trapClosing && trapState != trapOpenShow2  ) ||        // regular trigger press
     (fullTriggerState == LOW && triggerPress == false && trapState != trapOpening && trapState != trapClosing && trapState != trapOpenShow2 ) ||      // full trigger press
     (fullTriggerCycle == true && trapState != trapOpening && trapState != trapClosing && trapState != trapOpenShow2 ) ){                                                               // currently in full trigger cycle but ghost not trapped

    // trigger press of any kind detected only if we're not in a full trigger cycle
    if(fullTriggerCycle == false){
      Serial.println("Trigger press detected");
      triggerPress = true;
    }

    // If it's the full trigger press, flag it for automation. Don't press if it's already a full trigger cycle
    if (fullTriggerState == LOW && fullTriggerCycle == false){
      Serial.println("Full press detected");
      Serial.print("full trigger state is ");
      Serial.println(fullTriggerState);
      Serial.print("trigger state is ");
      Serial.println(triggerState);
      fullTriggerCycle = true;
    }

    // open doors and begin trap open sequence. Make it open half the time for the full sequence
    if (trapState == trapIdle) {
      if (fullTriggerCycle == true){
        doorTimer = millis() + (openTime/2);
      }
      else{
        doorTimer = millis() + openTime;
      }
      coroutines.start(OpenDoors);
      //COROUTINE_YIELD;
    }
    // Begin trap capture sequence
    else if ( (trapState == trapOpenShow1 && fullTriggerCycle == false) ||                    // normal trap cycle triggered from button press
      (trapState == trapOpenShow1 && fullTriggerCycle == true && millis() >= doorTimer) )      // full trap cycle triggered by timer being exceeded
    {
      coroutines.start(LightShow2);
      ghostTrapped = true;
      trapTimer = millis() + trapTime;
      
      // Now full trigger cycle is completed. We will no longer cycle through this portion of the loop to run full trigger sequences
      fullTriggerCycle = false;
      
    }

  }
  // This checks if the doors have been open for too long or the trap capture sequence has completed
  else if( ((trapState == trapOpenShow1) && millis() >= doorTimer) ||  ((trapState == trapOpenShow2) && millis() >= trapTimer)  ) {
     
    if((trapState == trapOpenShow1) && millis() >= doorTimer){
      Serial.println("Trap open too long. Auto closing.");
    }
    if((trapState == trapOpenShow2) && millis() >= trapTimer){
      Serial.println("Ghost captured. trap closing");
    }
    trapState = trapOpenIdle;
 
    coroutines.start(CloseDoors);
    

    //COROUTINE_YIELD;
   }
  // This detects if both buttons are let go but only if the trigger press was accepted (or done in a state where trigger press is tracked)
  // I used to detect each individually but the trigger press was resetting if only one was released. So now both must be released at once
  else if (triggerState == HIGH && fullTriggerState == HIGH && triggerPress == true) {
    triggerPress = false;
    fullTriggerState = HIGH;
    triggerState = HIGH;
    Serial.println("Trigger released");
  }


}


//
//
//  This coroutine will perform the following actions
//  1. Open the doors
//  2. Turn on light show 1 on pin 5
//  3. put out some initial smoke
//  4. Play the open trap sound effects
//
//
void OpenDoors(COROUTINE_CONTEXT(coroutine)) {
  BEGIN_COROUTINE;
  trapState = trapOpening;
  twinkleLoop = true;

  Serial.println("Doors opening and lights on");
  

  //light show sequence 1 goes here
  coroutines.start(Twinkle);               //Turn on the trap open lights
  COROUTINE_YIELD;
  coroutines.start(Smoke);                //Begin smoking
  COROUTINE_YIELD;
  coroutines.start(AudioTrigger);         //Play audio
  COROUTINE_YIELD;
  coroutine.wait(450);                    //give audio and everything else time to open along with the doors
  COROUTINE_YIELD;

  //open the doors
  AttachServos();
  MoveServos(servo1OpenPosition, servo2OpenPosition);
  coroutine.wait(doorWaitTime);
  COROUTINE_YIELD;
  DetachServos();

  trapState = trapOpenShow1;

  END_COROUTINE;
}

//
//
//  This coroutine will perform the following actions
//  1. Turn on the 5V light show stuff
//  2. click the 5v light show button twice
//  2. Put out some more smoke
//  3. Play the trap capture sound effects
//
//
void LightShow2(COROUTINE_CONTEXT(coroutine)) {

 
  BEGIN_COROUTINE;
 
  Serial.println("Trap sequence activated");
 
  trapState = trapOpenShow2;

  // Turn on the 5V light show
  digitalWrite(8, HIGH);
  coroutine.wait(300);
  COROUTINE_YIELD;    
  coroutines.start(ButtonPush5V);
  COROUTINE_YIELD; 

  coroutines.start(Smoke);                //Begin smoking
  COROUTINE_YIELD;
  coroutines.start(AudioTrigger);         //Play audio
  COROUTINE_YIELD;

  END_COROUTINE;
}

//
//
//  This coroutine will perform the following actions
//  1. Close the doors
//  2. Turn off all interior lights
//  2. Put out some more smoke
//  3. Play trap completion audio
//  4. Play trap completion lighting effects
//  5. Reset everything back to the default state
//
//
void CloseDoors(COROUTINE_CONTEXT(coroutine)) {
  BEGIN_COROUTINE;

   Serial.println("Trap close sequence");

  trapState = trapClosing;

  twinkleLoop = false;      // end the twinkle loop
  
  AttachServos();
  MoveServos(servo1ClosedPosition, servo2ClosedPosition);
  coroutine.wait(doorWaitTime);
  COROUTINE_YIELD;
  DetachServos();

  // turn off 5v light show LEDs. The other LEDs are turning off in the coroutine
  Serial.println("Turning off 5v lights");
  digitalWrite(8, LOW);

  if(ghostTrapped){
    coroutines.start(Smoke);                //Begin smoking
    COROUTINE_YIELD;
    coroutine.wait(5000);                    //wait 5 seconds
    COROUTINE_YIELD;
    coroutines.start(Smoke);                //more smoking
    COROUTINE_YIELD;


    //Run through exterior light sequence
    coroutines.start(OutsideLightShow);
    COROUTINE_YIELD; 

    coroutine.wait(5000);                    //wait 5 seconds
    COROUTINE_YIELD;
    coroutines.start(Smoke);                //more smoking
    COROUTINE_YIELD;
    coroutine.wait(5000);                    //wait 5 seconds
    COROUTINE_YIELD;
    coroutines.start(Smoke);                //more smoking
    COROUTINE_YIELD;


  }

  // shut off lights
  yellowLEDs[0] = CRGB::Black;
  yellowLEDs[1] = CRGB::Black;
  yellowLEDs[2] = CRGB::Black;
  yellowLEDs[3] = CRGB::Black;
  yellowLEDs[4] = CRGB::Black;
  redLED[0] = CRGB::Black;
  FastLED.show();

  // reset trigger states and variables
  doorTimer = 0;
  trapTimer = 0;
  ghostTrapped = false;
  triggerState = HIGH;
  fullTriggerState = HIGH;
  triggerPress = false;
  fullTriggerCycle = false;
  trapState = trapIdle;
  
  END_COROUTINE;
}

// This coroutine will run through the outside yellow and red light blinking
void OutsideLightShow(COROUTINE_CONTEXT(coroutine)) {
  COROUTINE_LOCAL(int, redBlink);  
  BEGIN_COROUTINE;
  
  Serial.println("Outside light show");

  //yellow bar
  coroutine.wait(500); 
  COROUTINE_YIELD;
  yellowLEDs[0] = CRGB::Orange;
  FastLED.show();
  coroutine.wait(100); 
  COROUTINE_YIELD;
  yellowLEDs[1] = CRGB::Orange;
  FastLED.show();
  coroutine.wait(100);   
  COROUTINE_YIELD;
  yellowLEDs[2] = CRGB::Orange;
  FastLED.show();
  coroutine.wait(100);
  COROUTINE_YIELD;
  yellowLEDs[3] = CRGB::Orange;
  FastLED.show();
  coroutine.wait(100);
  COROUTINE_YIELD;
  yellowLEDs[4] = CRGB::White;
  FastLED.show();
  
  coroutine.wait(1100);
  COROUTINE_YIELD;

  //Red blinking
  for(redBlink = 0; redBlink < 28; redBlink++){
    redLED[0] = CRGB::Red;
    FastLED.show();
    coroutine.wait(185);         
    COROUTINE_YIELD;
    redLED[0]  = CRGB::Black;
    FastLED.show();
    coroutine.wait(185);         
    COROUTINE_YIELD;
  }

  coroutine.wait(3000);
  COROUTINE_YIELD;
  
  // shut off lights
  yellowLEDs[0] = CRGB::Black;
  yellowLEDs[1] = CRGB::Black;
  yellowLEDs[2] = CRGB::Black;
  yellowLEDs[3] = CRGB::Black;
  yellowLEDs[4] = CRGB::Black;
  redLED[0] = CRGB::Black;
  FastLED.show();

  END_COROUTINE;
}



// This coroutine will cause the interior lights to activate for the trap open sequence
// Lights are on pin 5
void Twinkle(COROUTINE_CONTEXT(coroutine)) {
  COROUTINE_LOCAL(unsigned long, finishTime);
  COROUTINE_LOCAL(int, onTime);
  COROUTINE_LOCAL(int, offTime);
  COROUTINE_LOCAL(int, closing)

  BEGIN_COROUTINE;
  while(twinkleLoop){

    onTime = random(50, 200);       // Random time turned on
    offTime = random(50, 100);      // Random time turned off

    //on cycle with white lights on and purple lights on
    innerLEDs[0] = CRGB(0xFF00FF);
    innerLEDs[1] = CRGB(0xFF00FF);   
    innerLEDs[2] = CRGB(0xFF00FF);
    innerLEDs[3] = CRGB(0xFF00FF);
    innerLEDs[4] = CRGB(0xFF00FF);
    innerLEDs[5] = CRGB(0xFF00FF);
    FastLED.show();
    coroutine.wait(onTime);         // keep it on for a bit
    COROUTINE_YIELD;

    //off cycle with white lights on and purple lights off
    innerLEDs[0] = CRGB::White;
    innerLEDs[1] = CRGB::White;   
    innerLEDs[2] = CRGB::White;
    innerLEDs[3] = CRGB::White;
    innerLEDs[4] = CRGB::White;
    innerLEDs[5] = CRGB::White;
    FastLED.show();
    coroutine.wait(offTime);        // keep it on for a bit
    COROUTINE_YIELD;
  
  }

  finishTime = millis() + twinkleDelay;   // set some additional twinkle time
  
  // Twinkle for the additional time
  while(finishTime > millis() ){

    onTime = random(50, 200);       // Random time turned on
    offTime = random(50, 100);      // Random time turned off

    //on cycle with white lights on and purple lights on
    innerLEDs[0] = CRGB(0xFF00FF);
    innerLEDs[1] = CRGB(0xFF00FF);   
    innerLEDs[2] = CRGB(0xFF00FF);
    innerLEDs[3] = CRGB(0xFF00FF);
    innerLEDs[4] = CRGB(0xFF00FF);
    innerLEDs[5] = CRGB(0xFF00FF);
    FastLED.show();
    coroutine.wait(onTime);         // keep it on for a bit
    COROUTINE_YIELD;

    //off cycle with white lights on and purple lights off
    innerLEDs[0] = CRGB::White;
    innerLEDs[1] = CRGB::White;   
    innerLEDs[2] = CRGB::White;
    innerLEDs[3] = CRGB::White;
    innerLEDs[4] = CRGB::White;
    innerLEDs[5] = CRGB::White;
    FastLED.show();
    coroutine.wait(offTime);        // keep it on for a bit
    COROUTINE_YIELD;
  
  }
  

  // Turn off lights here
  fill_solid(innerLEDs, 6, CRGB::Black);
  FastLED.show();

  END_COROUTINE;
}

// This coroutine will push the button for the 5v lights twice]
// it is no longer being used but is left here during troubleshooting
void ButtonPush5V(COROUTINE_CONTEXT(coroutine)) {
  
  BEGIN_COROUTINE;
/*******************************
  //push the button, hold it for 200ms, wait 300ms, then push it for 200ms again
  digitalWrite(8, HIGH);
  coroutine.wait(200);
  COROUTINE_YIELD;
  digitalWrite(8, LOW);
  coroutine.wait(300);
  COROUTINE_YIELD;
  digitalWrite(8, HIGH);
  coroutine.wait(200);
  COROUTINE_YIELD;
  digitalWrite(8, LOW);
  coroutine.wait(200);
  COROUTINE_YIELD;
*******************************/

  END_COROUTINE;
}

// This coroutine will simulate the button push needed for the audio triggers
void AudioTrigger(COROUTINE_CONTEXT(coroutine)) {
  COROUTINE_LOCAL(int, audioFile);

  BEGIN_COROUTINE;

  Serial.print(trapState);
  audioFile = 0;

  if (trapState == trapOpening){
    Serial.println(" Audio 1.");
    audioFile = 14;   
     
  }
  else if (trapState == trapOpenShow2){
    Serial.println(" Audio 2.");
    audioFile = 15;
  }
  Serial.print("pin ");
  Serial.print(audioFile);
  Serial.println(" should be used for audio here");

  //push the button, hold it for 200ms
  digitalWrite(audioFile, HIGH);
  coroutine.wait(200);
  COROUTINE_YIELD;
  digitalWrite(audioFile, LOW);
  
  END_COROUTINE;
}

// This coroutine start the smoke machine for a period of time
void Smoke(COROUTINE_CONTEXT(coroutine)) {
  BEGIN_COROUTINE;

  digitalWrite(7, HIGH);
  coroutine.wait(smokeTime);
  COROUTINE_YIELD;
  digitalWrite(7, LOW);
  
  END_COROUTINE;
}

void AttachServos() {
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
}

void MoveServos(int servo1Position, int servo2Position) {
  servo1.write(servo1Position);
  servo2.write(servo2Position);
}

void DetachServos() {
  servo1.detach();
  servo2.detach();
}