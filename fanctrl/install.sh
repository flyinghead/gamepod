#!/bin/bash
apt-get install pigpio python-pigpio python3-pigpio
systemctl enable pigpiod
systemctl start pigpiod

install fanctrl.py /usr/local/bin
install fan-control.service /etc/systemd/system
systemctl enable fan-control
systemctl restart fan-control
