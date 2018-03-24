#!/bin/bash
install othermodoff.py /usr/local/bin
install othermodoff.service /etc/systemd/system
systemctl enable othermodoff.service
systemctl restart othermodoff.service
