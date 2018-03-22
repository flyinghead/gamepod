#!/usr/bin/env python
import os
import time
from time import sleep
import signal
import sys
import RPi.GPIO as GPIO

desiredTemp = 75 # The maximum temperature in Celsius after which we trigger the fan
		 # Max temp for BCM2837 is 85 C. Pi thermal throttle probably around 82 C

# PI(D) params
pTemp=15
iTemp=0.6

fanSpeed=100
sum=0

def getCPUtemperature():
    res = os.popen('vcgencmd measure_temp').readline()
    temp =(res.replace("temp=","").replace("'C\n",""))
    #print("temp is {0}".format(temp)) #Uncomment here for testing
    return temp

def handleFan():
    global fanSpeed,sum
    actualTemp = float(getCPUtemperature())
    diff=actualTemp-desiredTemp
    sum=sum+diff
    pDiff=diff*pTemp
    iDiff=sum*iTemp
    fanSpeed=pDiff +iDiff
    if fanSpeed>100:
        fanSpeed=100
    if fanSpeed<0:
        fanSpeed=0
    if sum>100:
        sum=100
    if sum<-50:
        sum=-50
    #print("actualTemp %4.2f TempDiff %4.2f pDiff %4.2f iDiff %4.2f fanSpeed %5d" % (actualTemp,diff,pDiff,iDiff,fanSpeed))
    Pwm.ChangeDutyCycle(int(fanSpeed))
    return()

def fanOFF():
    Pwm.ChangeDutyCycle(0)   # switch fan off
    return()

def signalHandler(_signo, _stack_frame):
    fanOFF()
    sys.exit(0)

try:
    signal.signal(signal.SIGTERM, signalHandler)
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(12, GPIO.OUT)
    Pwm = GPIO.PWM(12, 23000)
    Pwm.start(50)
    fanOFF()
    while True:
        handleFan()
        sleep(3) # Read the temperature every 3 sec
except KeyboardInterrupt: # trap a CTRL+C keyboard interrupt 
    fanOFF()
#    GPIO.cleanup() # resets all GPIO ports used by this program
