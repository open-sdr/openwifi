#!/bin/bash
set -x

rm BOOT.BIN
sync
wget ftp://192.168.10.1/kernel_boot/output_boot_bin/BOOT.BIN
sync
rm uImage
sync
wget ftp://192.168.10.1/adi-linux/arch/arm/boot/uImage
sync
rm devicetree.dtb
sync
wget ftp://192.168.10.1/kernel_boot/devicetree.dtb
sync
#slepp 0.5
mount /dev/mmcblk0p1  /sdcard
sync
#sleep 0.5
cp BOOT.BIN /sdcard/ -f
cp uImage /sdcard/ -f
cp devicetree.dtb /sdcard/ -f
sync
#sleep 0.5
umount /sdcard
sync
#sleep 3
sudo reboot now

