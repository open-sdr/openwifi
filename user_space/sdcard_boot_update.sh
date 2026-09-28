#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -ne 1 ]; then
    echo "You must enter the \$BOARD_NAME as argument"
    echo "Like: sdrpi antsdr antsdr_e200 e310v2 adrv9364z7020 adrv9361z7035 zc706_fmcs2 zed_fmcs2 zc702_fmcs2 zcu102_fmcs2 zcu102_9371 neptunesdr"
    exit 1
fi
BOARD_NAME=$1

if [ "$BOARD_NAME" != "neptunesdr" ] && [ "$BOARD_NAME" != "antsdr" ] && [ "$BOARD_NAME" != "antsdr_e200" ] && [ "$BOARD_NAME" != "e310v2" ] && [ "$BOARD_NAME" != "sdrpi" ] && [ "$BOARD_NAME" != "zc706_fmcs2" ] && [ "$BOARD_NAME" != "zc702_fmcs2" ] && [ "$BOARD_NAME" != "zed_fmcs2" ] && [ "$BOARD_NAME" != "adrv9361z7035" ] && [ "$BOARD_NAME" != "adrv9364z7020" ] && [ "$BOARD_NAME" != "zcu102_fmcs2" ] && [ "$BOARD_NAME" != "zcu102_9371" ]; then
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

mv BOOT.BIN BOOT.BIN.bak
sync
wget ftp://192.168.10.1/kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN
if [ -f "./BOOT.BIN" ]; then
    echo "BOOT.BIN downloaded!"
else
    echo "WARNING! BOOT.BIN not downloaded! Old file used!"
    mv BOOT.BIN.bak BOOT.BIN
#    exit 1
fi
sync

mv $image_filename $image_filename.bak
sync
wget ftp://192.168.10.1/$LINUX_KERNEL_SRC_DIR_NAME/$image_filepath/$image_filename
if [ -f "./$image_filename" ]; then
    echo "$image_filename downloaded!"
else
    echo "WARNING! $image_filename not downloaded! Old file used!"
    mv $image_filename.bak $image_filename
#    exit 1
fi
sync

mv $dtb_filename $dtb_filename.bak
sync
wget ftp://192.168.10.1/kernel_boot/boards/$BOARD_NAME/$dtb_filename
if [ -f "./$dtb_filename" ]; then
    echo "$dtb_filename downloaded!"
else
    echo "WARNING! $dtb_filename not downloaded! Old file used!"
    mv $dtb_filename.bak $dtb_filename
#    exit 1
fi
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
