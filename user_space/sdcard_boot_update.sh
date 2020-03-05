#!/bin/bash
if [ "$#" -ne 1 ]; then
    echo "You must enter the \$BOARD_NAME as argument"
    echo "Like: adrv9364z7020 adrv9361z7035 adrv9361z7035_fmc zc706_fmcs2 zed_fmcs2 zc702_fmcs2"
    exit 1
fi
BOARD_NAME=$1

set -x

rm BOOT.BIN
sync
wget ftp://192.168.10.1/kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN
sync
rm uImage
sync
wget ftp://192.168.10.1/adi-linux/arch/arm/boot/uImage
sync
rm devicetree.dtb
sync
wget ftp://192.168.10.1/kernel_boot/boards/$BOARD_NAME/devicetree.dtb
sync
#slepp 0.5
mount /dev/mmcblk0p1  /mnt
sync
#sleep 0.5
cp BOOT.BIN /mnt/ -f
cp uImage /mnt/ -f
cp devicetree.dtb /mnt/ -f
sync
#sleep 0.5
umount /mnt
sync
#sleep 3
sudo reboot now

