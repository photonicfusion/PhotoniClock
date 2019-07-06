@echo off
set path=%~dp0

%~dp0\avrdude.exe -v -patmega328p -cusbtiny -e -Ulock:w:0x3F:m -Uefuse:w:0xFD:m -Uhfuse:w:0xDE:m -Ulfuse:w:0xFF:m
%~dp0\avrdude.exe -v -patmega328p -cusbtiny -Uflash:w:%path%/hex/optiboot_atmega328.hex:i -Ulock:w:0x0F:m
