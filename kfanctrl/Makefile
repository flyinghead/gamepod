all: pwm-fan.dtbo

pwm-fan.dtbo: pwm-fan.dtbs
	dtc -@ -I dts -O dtb -o pwm-fan.dtbo pwm-fan.dtbs

install: pwm-fan.dtbo
	cp pwm-fan.dtbo /boot/overlays

clean:
	rm -f pwm-fan.dtbo

