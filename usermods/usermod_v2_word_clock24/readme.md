# Word Clock Usermod - 24 hours

This usermod can be used to drive a wordclock with a 11x11 pixel matrix with WLED for 24 hours. There are also 4 additional dots for the minutes. 
The visualisation is desribed in 4 mask with LED numbers (single dots for minutes, minutes, hours and "clock/Uhr").
There are 2 parameters to chnage the behaviour:

# Usage 

Copy `platformio_override.ini.sample` to project root an rename it to `platformio_override.ini`.
The default settings compiles WLED for a ESP8266 (D1 mini). Compile and flahs this version using the arrow icon in the status bar.
