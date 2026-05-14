# Word Clock Usermod V2

This usermod can be used to drive a wordclock with a 11x10 pixel matrix with WLED. There are also 4 additional dots for the minutes. 
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
Copy and update the example `platformio_override.ini` from this to root directory of WLED project.

## Build
Manually build for platforms defined in the override file.
1. Click alien symbol on the left
2. Quick Access -> Miscellaneous -> PlatformIO Core CLI
3. Insert command in terminal   
```pio run -e WC_esp32c3 ; pio run -e WC_generic8266```

## Change Log
2026/05/14 Ported to V16
2022/08/18 added meander wiring pattern.
2022/03/30 initial commit
