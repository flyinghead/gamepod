all: pwm-fan.dtbo

pwm-fan.dtbo: pwm-fan.dtbs
	dtc -@ -I dts -O dtb -o pwm-fan.dtbo pwm-fan.dtbs

install: pwm-fan.dtbo
	cp pwm-fan.dtbo /boot/overlays
	install fanctrl.py /usr/local/bin
	install -m 644 fan-control.service /etc/systemd/system
	systemctl enable fan-control
	systemctl restart fan-control

clean:
	rm -f pwm-fan.dtbo

