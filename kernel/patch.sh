#!/bin/bash

set -e

sudo apt-get install bc
wget https://github.com/raspberrypi/linux/archive/be2540e540f5442d7b372208787fb64100af0c54.zip
unzip be2540e540f5442d7b372208787fb64100af0c54.zip
cd linux-be2540e540f5442d7b372208787fb64100af0c54
patch -p1 <../gpio-poweroff.patch
KERNEL=kernel7
make bcm2709_defconfig
#CONFIG_SENSORS_PWM_FAN=m CONFIG_BACKLIGHT_PWM=y

grep -v '^CONFIG_SENSORS_PWM_FAN=' .config > .config.tmp
echo CONFIG_SENSORS_PWM_FAN=m >> .config.tmp
mv .config.tmp .config

grep -v '^CONFIG_BACKLIGHT_PWM=' .config > .config.tmp
echo CONFIG_BACKLIGHT_PWM=m >> .config.tmp
mv .config.tmp .config

make -j4 LOCALVERSION=+ zImage modules

echo Installing kernel image in /boot as sudo
sudo cp arch/arm/boot/zImage /boot/kernel7.img

echo Installing kernel modules in /lib as sudo
sudo make modules_install
