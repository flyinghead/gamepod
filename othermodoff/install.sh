#!/bin/bash
install othermodoff.py /usr/local/bin
install -m 644 othermodoff.service /etc/systemd/system
systemctl enable othermodoff.service
systemctl restart othermodoff.service
