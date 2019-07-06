#!/bin/bash

BASEDIR=$(dirname "$0")

avrdude -q -q -c usbtiny -p atmega328p -B 1

