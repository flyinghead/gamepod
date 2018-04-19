all: pwm-backlight.dtbo

pwm-backlight.dtbo: pwm-backlight.dtbs
	dtc -@ -I dts -O dtb -o pwm-backlight.dtbo pwm-backlight.dtbs

install: pwm-backlight.dtbo
	cp pwm-backlight.dtbo /boot/overlays
	install -m 644 pwm-backlight.service /etc/systemd/system
	systemctl enable pwm-backlight
	systemctl restart pwm-backlight

clean:
	rm -f pwm-backlight.dtbo

