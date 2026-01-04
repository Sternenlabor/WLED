# Word Clock Usermod V2

This usermod can be used to drive a wordclock with a 11x11 pixel matrix with WLED. There are also 4 additional dots for the minutes. 
The visualisation is desribed in 4 mask with LED numbers (single dots for minutes, minutes, hours and "clock/Uhr").
There are 2 parameters to chnage the behaviour:
 
active: enable/disable usermod
diplayItIs: enable/disable display of "Es ist" on the clock.

### Update for alternatative wiring pattern
Based on this fantastic work I added an alternative wiring pattern.
For original you have to use a long wire to connect DO - DI from first line to the next line.

I wired my clock in meander style. So the first LED in second line is in the right.
With this problem every second line was inverted and showed the wrong letter.

I added a switch in usermod called "meander wiring?" to enable/disable alternativ wiring pattern.


## Installation

Einfach mal so bauen. Aktuell gibt es ein env:usermod, da werden alle 
usermods mitgenommen. Dieses ist dann auch dabei.
Alternativ (um nur dieses zu haben)
Kopiere diese `platformio_override.ini`
von diesem Usermod in den root-Folder des Builds.
Diese Datei sollte im selben Vz sein wie `platformio.ini`.
Dadurch wird der default auf `d1_wordclock24` und diese Config
wird gebaut. 

Hier wird allerdings auf den ESP32 abgezielt. Im konkreten Fall auf espc3. 
Entsprechend diese Zeile in der `platformio_override.ini` anpassen.


### Define Your Options

 none

### PlatformIO requirements

No special requirements.

## Change Log

2025-12-24 Port auf neues Usermod system

2022-08-18 added meander wiring pattern.

2022-03-30 initial commit
