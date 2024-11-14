# DCF77 user modification

 This usermod allows you to get date and time from a DCF77 module. This is usefull if 
 you don't have access to wifi / NTP.   
 The usermod uses a modified version of the DCF77 arduino library https://github.com/thijse/Arduino-DCF77.
  
 The mod is tested with a D1 mini. I have no idea if it works with other controllers.
 
 
 You **HAVE TO** setup the T (Signal) GPIO in usermod settings over the webinterface.
  
 
  
 If you have any questions, visit us at the *star laboratory* (http://sternenlabor.de) in Plauen.

## Wiring
Connect the DCF77 modul as follows:
- V to 3.3V
- GND to GND
- P1 (Enable) to GND
- T (Signal) to e.g. GPIO15 - (D8 on D1 mini) 


## Installation
To enable this mod, add it to usermods_list.cpp with the define *USERMOD_DCF77*.
Also add suitable USERMOD_ID_DCF77 to const.h.
To enable the module:
- add *-DUSERMOD_DCF77* to your platformio_override.ini
- add *-DPIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48*  (otherwise you'll get a compile error)

