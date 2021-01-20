#!/bin/bash

# SPDX-FileCopyrightText: 2019 Jiao Xianjun <putaoshu@msn.com>
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -ne 1 ]; then
    echo "You must enter the \$BOARD_NAME as argument"
    echo "Like: adrv9364z7020 adrv9361z7035 zc706_fmcs2 zed_fmcs2 zc702_fmcs2 zcu102_fmcs2 zcu102_9371"
    exit 1
fi
BOARD_NAME=$1

if [ "$BOARD_NAME" != "zc706_fmcs2" ] && [ "$BOARD_NAME" != "zc702_fmcs2" ] && [ "$BOARD_NAME" != "zed_fmcs2" ] && [ "$BOARD_NAME" != "adrv9361z7035" ] && [ "$BOARD_NAME" != "adrv9364z7020" ] && [ "$BOARD_NAME" != "zcu102_fmcs2" ] && [ "$BOARD_NAME" != "zcu102_9371" ]; then
    echo "\$BOARD_NAME is not correct. Please check!"
    exit 1
else
    echo "\$BOARD_NAME is found!"
fi

if [ "$BOARD_NAME" == "zcu102_fmcs2" ] || [ "$BOARD_NAME" == "zcu102_9371" ]; then
    dtb_filename="system.dtb"
    image_filepath="arch/arm64/boot/"
    image_filename="Image"
    LINUX_KERNEL_SRC_DIR_NAME=adi-linux-64
else
    dtb_filename="devicetree.dtb"
    image_filepath="arch/arm/boot/"
    image_filename="uImage"
    LINUX_KERNEL_SRC_DIR_NAME=adi-linux
fi
echo $dtb_filename
echo $image_filepath
echo $image_filename

set -x

rm BOOT.BIN
sync
wget ftp://192.168.10.1/kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN
sync
rm $image_filename
sync
wget ftp://192.168.10.1/$LINUX_KERNEL_SRC_DIR_NAME/$image_filepath/$image_filename
sync
rm $dtb_filename
sync
wget ftp://192.168.10.1/kernel_boot/boards/$BOARD_NAME/$dtb_filename
sync
#slepp 0.5
mount /dev/mmcblk0p1  /mnt
sync
#sleep 0.5
cp BOOT.BIN /mnt/ -f
rm /mnt/Image -f
rm /mnt/uImage -f
cp $image_filename /mnt/ -f
rm /mnt/*.dtb -f
cp $dtb_filename /mnt/ -f
cd /mnt/
sync
cd ~
#sleep 0.5
umount /mnt
sync
#sleep 3
sudo reboot now
