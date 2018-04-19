#!/bin/bash

set -e

function make_and_install_kernel() {
	make -j4 LOCALVERSION=+ zImage

	echo Installing kernel image in /boot with sudo
	sudo cp arch/arm/boot/zImage /boot/kernel7.img
}

sudo apt-get install build-essential libncurses5-dev bc

FIRMWARE_HASH=$(zgrep "* firmware as of" /usr/share/doc/raspberrypi-bootloader/changelog.Debian.gz | head -1 | awk '{ print $5 }')
# get git hash for this kernel
KERNEL_HASH=$(wget https://raw.github.com/raspberrypi/firmware/$FIRMWARE_HASH/extra/git_hash -O -)

wget https://github.com/raspberrypi/linux/archive/${KERNEL_HASH}.zip
unzip -o ${KERNEL_HASH}.zip
cd linux-${KERNEL_HASH}

patch -p1 <../gpio-poweroff.patch
KERNEL=kernel7
make bcm2709_defconfig

if lsb_release -c | grep -q jessie ; then
	make_and_install_kernel
else
	grep -v '^CONFIG_SENSORS_PWM_FAN=' .config > .config.tmp
	echo CONFIG_SENSORS_PWM_FAN=m >> .config.tmp
	mv .config.tmp .config

	grep -v '^CONFIG_BACKLIGHT_PWM=' .config > .config.tmp
	echo CONFIG_BACKLIGHT_PWM=m >> .config.tmp
	mv .config.tmp .config

	make_and_install_kernel

	make -j4 LOCALVERSION=+ modules

	echo Installing kernel modules in /lib with sudo
	sudo make modules_install
fi
