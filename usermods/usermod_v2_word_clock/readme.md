# Word Clock Usermod V2

This usermod drives an 11x10 pixel matrix wordclock with WLED. There are 4 additional dots for the minutes.
The visualisation is described by 4 masks with LED numbers (single dots for minutes, minutes, hours and "clock"). The index of the LEDs in the masks always starts at 0, even if the ledOffset is not 0.
There are 3 parameters that control behavior:

active: enable/disable usermod
diplayItIs: enable/disable display of "Es ist" on the clock
ledOffset: number of LEDs before the wordclock LEDs

## Update for alternative wiring pattern

Based on this fantastic work I added an alternative wiring pattern.
The original used a long wire to connect DO to DI, from one line to the next line.

I wired my clock in meander style. So the first LED in the second line is on the right.
With this method, every other line was inverted and showed the wrong letter.

I added a switch in usermod called "meander wiring?" to enable/disable the alternate wiring pattern.

## Installation

Einfach mal so bauen. Aktuell gibt es ein env:usermod, da werden alle 
usermods mitgenommen. Dieses ist dann auch dabei.
Alternativ (um nur dieses zu haben)
Kopiere diese `platformio_override.ini`
von diesem Usermod in den root-Folder des Builds.
Dadurch wird der default auf `usermod_wordclock24` und diese Config
wird gebaut. 

Hier wird allerdings auf den ESP32 abgezielt. Im konkreten Fall auf espc3. 
Entsprechend diese Zeile in der `platformio_override.ini` anpassen.

### Define Your Options

* nix nötig

### PlatformIO requirements

No special requirements.

## Change Log

2025-12-24 Port auf neues Usermod system

2022/08/18 added meander wiring pattern.

2022/03/30 initial commit
