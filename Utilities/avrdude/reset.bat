@echo off
%~dp0\avrdude.exe -q -q -c usbtiny -p atmega328p -B 1
