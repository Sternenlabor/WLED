# How to compile
- Copy *platformio_override.ini* to root of project
- Start compile by pressing the upload -> button
  - platformio will download all libraries
  - build will fail after download
- Overwrite file *.pio/libdeps/lilygo/TFT_eSPI/User_Setup.h* by file *User_Setup.h* in lilygo usermod directory.
- Compile and flash by pressing the upload -> button

# Technical data
- Display resolution: 320x170
- LED strip on: PIN10

