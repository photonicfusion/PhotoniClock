#!/bin/bash

path=$1
com=$2
BASEDIR=$(dirname "$0")

avrdude -v -patmega328p -cusbtiny -e -Ulock:w:0x3F:m -Uefuse:w:0xFD:m -Uhfuse:w:0xDE:m -Ulfuse:w:0xFF:m
avrdude -v -patmega328p -cusbtiny -Uflash:w:${BASEDIR}/hex/optiboot_atmega328.hex:i -Ulock:w:0x0F:m
