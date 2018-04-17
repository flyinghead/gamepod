#!/usr/bin/env python
import os
import time
from time import sleep
import signal
import sys

desiredTemp = 75 # The maximum temperature in Celsius after which we trigger the fan
		 # Max temp for BCM2837 is 85 C. Pi thermal throttle is at 80 C

# PI(D) params
pTemp=15
iTemp=0.6

fanSpeed=99
sum=0

def getCPUtemperature():
    with open('/sys/class/thermal/thermal_zone0/temp','r') as f:
        res = f.read()
    temp = float(res) / 1000
    #print("temp is {0}".format(temp)) #Uncomment here for testing
    return temp

def setFanSpeed(fanSpeed):
    with open('/sys/class/thermal/cooling_device0/cur_state','w') as f:
	f.write(str(fanSpeed))


def handleFan():
    global fanSpeed, sum
    actualTemp = getCPUtemperature()
    diff = actualTemp - desiredTemp
    sum = sum + diff
    pDiff = diff * pTemp
    iDiff = sum * iTemp
    fanSpeed = pDiff + iDiff
    if fanSpeed > 99:
        fanSpeed = 99
    if fanSpeed < 0:
        fanSpeed = 0
    if sum > 100:
        sum = 100
    if sum < -50:
        sum = -50
    # TODO Read max_state instead of harcoding it
    fanSpeed = round(fanSpeed / 10, 0)
    if fanSpeed > 9:
        fanSpeed = 9
    #print("actualTemp %4.2f TempDiff %4.2f pDiff %4.2f iDiff %4.2f fanSpeed %2d" % (actualTemp, diff, pDiff, iDiff, fanSpeed))
    setFanSpeed(fanSpeed)
    return()

def fanOFF():
    setFanSpeed(0)
    return()

def signalHandler(_signo, _stack_frame):
    fanOFF()
    sys.exit(0)

try:
    signal.signal(signal.SIGTERM, signalHandler)
    fanOFF()
    while True:
        handleFan()
        sleep(3) # Read the temperature every 3 sec
except KeyboardInterrupt: # trap a CTRL+C keyboard interrupt 
    fanOFF()
