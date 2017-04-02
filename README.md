# arduino

some little bits of code written in c that run on an arduino.

### Dependencies
+ avr-libc
+ avrdude
+ gcc-avr
+ gdb+avr
+ binutils-avr

see http://avr-eclipse.sourceforge.net/wiki/index.php/The_AVR_GCC_Toolchain for more details about installing avr utilities on various platforms.

### Process

1. usb up the arduino (I've tested on an uno and nano)  
2. cd to the project directory  
3. build  
    ../utils/build
4. listen for serial output  
    ../utils/listen
5. to build and immediately listen:  
    ../utils/build && ../utils/listen
    
### Finding your arduinos terminal

The terminal used by your arduino may differ from the build scripts, to check run:  
    ls /dev/
before and after attaching the device; you should see the list change.
