#!/bin/bash
PORT=/dev/ttyACM0

python arduinoreset.py $PORT
sleep 2
avrdude -Cavrdude.conf -v -patmega32u4 -cavr109 -P$PORT -b57600 -D -Uflash:w:joystick.ino.promicro.hex:i
