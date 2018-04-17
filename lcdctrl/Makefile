all: pwm-backlight.dtbo

pwm-backlight.dtbo: pwm-backlight.dtbs
	dtc -@ -I dts -O dtb -o pwm-backlight.dtbo pwm-backlight.dtbs

install: pwm-backlight.dtbo
	cp pwm-backlight.dtbo /boot/overlays

clean:
	rm -f pwm-backlight.dtbo

