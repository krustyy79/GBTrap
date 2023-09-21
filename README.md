# GBTrap
 Arduino Nano code for converting a spirit halloween ghost trap

This is the code for modifying a spirit halloween ghost trap to function fully remotely. All the internals were rebuilt for use with arduino nano. The following parts were also included in the build related to this code

4 channel logic level MOSFET board for triggering various functions.
https://www.tindie.com/products/drazzy/4-channel-logic-level-mosfet-boards/

MP3 player with 5W audio amp
https://gravedecor.com/product/mp3-player-with-5w-audio-amp/

I also used some USB (5v) laser and light modules for lighting effects

Buck converters for voltage regulation since I'm using 5v and 3.3v
https://www.mpja.com/LM2596-Step-Down-Adjustable-15-37V-DC_DC-Converter/productinfo/30148+PS/

3vdc mini air pumps for smoke
https://www.amazon.com/Aquarium-Oxygen-Circulate-Sphygmomanometer-Massager/dp/B0BFBRSCV9/ref=sr_1_8?crid=1693LFFW7XXCL&keywords=3vdc+mini+air+pump&qid=1695333666&sprefix=3vdc+mini+air+pump%2Caps%2C129&sr=8-8

a cheap vape pen to tear apart, run at 3.3v, and hook up the mini pump to.

Some micro servos
https://www.amazon.com/gp/product/B09Y55C21K/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1

WS2812B RGB 5050SMD individually addressable LED strip
https://www.amazon.com/gp/product/B01CDTEJR0/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1

This code also makes use of coroutines and fastled which need to be included to run