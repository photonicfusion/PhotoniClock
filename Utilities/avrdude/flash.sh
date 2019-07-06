#!/bin/bash

path=$1
com=$2
BASEDIR=$(dirname "$0")

if [ "${com}" = "USBTiny" ]; then
    #Flash with bootloader, otherwise the chip fails to flash using UART for some reason
    avrdude -p atmega328p -c usbtiny -B 1 -V -D -U flash:w:"${path}":i
else
    avrdude -p atmega328p -c arduino -P ${com} -b 115200 -D -U flash:w:"${path}":i
fi
