#!/usr/bin/env python
  
import RPi.GPIO as GPIO  
import time  
import os  

GPIO_PIN=4

GPIO.setmode(GPIO.BCM)  
GPIO.setup(GPIO_PIN, GPIO.IN, pull_up_down = GPIO.PUD_UP)  
 
def Shutdown():  
    os.system("halt --no-wall")

def PowerButtonDown(channel):
    start = time.time()
    while not GPIO.input(GPIO_PIN):
        if time.time() - start >= 1.0:
	    Shutdown()
	    return()
	time.sleep(0.1)

GPIO.add_event_detect(GPIO_PIN, GPIO.FALLING, callback = PowerButtonDown)  
 
while 1:  
    time.sleep(3600)
