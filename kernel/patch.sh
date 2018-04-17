#!/bin/bash

set -e

sudo apt-get install bc
wget https://github.com/raspberrypi/linux/archive/be2540e540f5442d7b372208787fb64100af0c54.zip
unzip be2540e540f5442d7b372208787fb64100af0c54.zip
cd linux-be2540e540f5442d7b372208787fb64100af0c54
patch -p1 <../gpio-poweroff.patch
KERNEL=kernel7
make bcm2709_defconfig
make -j4 LOCALVERSION=+ zImage
echo Installing kernel image in /boot as sudo
sudo cp arch/arm/boot/zImage /boot/kernel7.img
