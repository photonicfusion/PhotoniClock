# PhotoniClock
PhotoniClock firmware, and software utilities.

Follow this guide to build the source and flash to the PhotoniClock.

The PhotoniClock firmware can be built using two methods:
1. Using a containerized environment with Docker.
2. Using the Arduino IDE (after updating AVR-GCC).


[Method 1] Docker
-----------------------------------------------
1. Install Docker CE on your build system: https://docs.docker.com/install/
2. Install the Arduino Builder for Docker: https://hub.docker.com/r/photonicfusion/arduino_builder
3. Download additional libraries:
  - https://github.com/nitacku/nAudio
  - https://github.com/nitacku/nCoder
  - https://github.com/nitacku/nDisplay
  - https://github.com/nitacku/nEEPROM
  - https://github.com/nitacku/nI2C
  - https://github.com/nitacku/nRTC
  - https://github.com/photonicfusion/nStd
  - https://github.com/FastLED/FastLED
4. Create a directory structure with top level named "PhotoniClock" and two sub-directories named "src" and "libs".
5. Extract the libraries downloaded from the previous step into the "libs" directory.
6. Download the PhotoniClock source and copy /Firmware/PhotoniClock to the "src" directory.
7. Save one of the convenience scripts provided on the Arduino Builder to the PhotoniClock directory.
8. Run the convenience script to build the source.
9. If the build completed successfully, a .hex file will be located in the "src" directory.
10. Connect an FTDI cable from your computer to the 6-pin header on the bottom of the PhotoniClock board, matching the pin labled "GND-BLACK" to the black wire of the FTDI cable.
11. Use the utility scripts provided with the PhotoniClock source (Utility directory) to flash the .hex file to the PhotoniClock.


[Method 2] Arduino IDE
-----------------------------------------------
1. Install the latest version of the Arduino IDE: https://www.arduino.cc/en/Main/Software
2. Update AVR-GCC used by Arduino to a version >=8.1. Follow this guide: http://blog.zakkemble.net/avr-gcc-builds/
3. Download additional libraries:
  - https://github.com/nitacku/nAudio
  - https://github.com/nitacku/nCoder
  - https://github.com/nitacku/nDisplay
  - https://github.com/nitacku/nEEPROM
  - https://github.com/nitacku/nI2C
  - https://github.com/nitacku/nRTC
  - https://github.com/photonicfusion/nStd
  - https://github.com/FastLED/FastLED
4. Follow this guide to install downloaded libraries to Arduino: https://www.arduino.cc/en/Guide/Libraries#toc4
5. Download the PhotoniClock source and open /Firmware/PhotoniClock/PhotoniClock.ino with the Arduino IDE.
6. Press the build button in the Arduino IDE to build the source. If everything is correct, the build will complete without error.
7. Connect an FTDI cable from your computer to the 6-pin header on the bottom of the PhotoniClock board, matching the pin labled "GND-BLACK" to the black wire of the FTDI cable.
8. Press the upload button in the Arduino IDE to flash the PhotoniClock.

