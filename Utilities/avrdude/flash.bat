@echo off
set path=%1
set com=%2

if "%com%" == "USBTiny" (
    %~dp0\avrdude.exe -p atmega328p -c usbtiny -B 1 -V -D -U flash:w:"%path%":i
) else (
    %~dp0\avrdude.exe -p atmega328p -c arduino -P %com% -b 115200 -D -U flash:w:"%path%":i
)
