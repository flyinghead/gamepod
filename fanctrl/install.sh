#!/bin/bash
install fanctrl.py /usr/local/bin
install fan-control.service /etc/systemd/system
systemctl enable fan-control.service
systemctl restart fan-control.service
