  
#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

# Setup Eth connection before this script!
# Host: 192.168.10.1
# Board: 192.168.10.122
# Commands onboard to setup:
# ifconfig eth0 192.168.10.122 netmask 255.255.255.0
# ifconfig eth0 up
# route add default gw 192.168.10.1

if [ "$#" -ne 2 ]; then
    echo "You have input $# arguments."
    echo "You must enter \$DIR_TO_ADI_LINUX_KERNEL and \$BOARD_NAME as argument"
    exit 1
fi

DIR_TO_ADI_LINUX_KERNEL=$1
BOARD_NAME=$2

if [ "$BOARD_NAME" != "antsdr" ] && [ "$BOARD_NAME" != "antsdr_e200" ] && [ "$BOARD_NAME" != "e310v2" ] && [ "$BOARD_NAME" != "sdrpi" ] && [ "$BOARD_NAME" != "neptunesdr" ] && [ "$BOARD_NAME" != "zc706_fmcs2" ] && [ "$BOARD_NAME" != "zc702_fmcs2" ] && [ "$BOARD_NAME" != "zed_fmcs2" ] && [ "$BOARD_NAME" != "adrv9361z7035" ] && [ "$BOARD_NAME" != "adrv9364z7020" ] && [ "$BOARD_NAME" != "zcu102_fmcs2" ]; then
    echo "\$BOARD_NAME is not correct. Please check!"
    exit 1
else
    echo "\$BOARD_NAME is found!"
fi

if [ "$BOARD_NAME" == "zcu102_fmcs2" ] || [ "$BOARD_NAME" == "zcu102_9371" ]; then
    LINUX_KERNEL_IMAGE=$DIR_TO_ADI_LINUX_KERNEL/arch/arm64/boot/Image
    DTB_FILENAME="system.dtb"
else
    LINUX_KERNEL_IMAGE=$DIR_TO_ADI_LINUX_KERNEL/arch/arm/boot/uImage
    DTB_FILENAME="devicetree.dtb"
fi

mkdir -p kernel_modules
rm -rf ./kernel_modules/*
find $DIR_TO_ADI_LINUX_KERNEL/ -name \*.ko -exec cp {} ./kernel_modules/ \;

cp $DIR_TO_ADI_LINUX_KERNEL/Module.symvers ./kernel_modules/
cp $DIR_TO_ADI_LINUX_KERNEL/modules.builtin ./kernel_modules/
cp $DIR_TO_ADI_LINUX_KERNEL/modules.builtin.modinfo ./kernel_modules/
cp $DIR_TO_ADI_LINUX_KERNEL/modules.order ./kernel_modules/

if test -f "$LINUX_KERNEL_IMAGE"; then
    cp $LINUX_KERNEL_IMAGE ./kernel_modules/
fi

if test -f "../kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN"; then
    cp ../kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN ./kernel_modules/
fi
if test -f "../kernel_boot/boards/$BOARD_NAME/$DTB_FILENAME"; then
    cp ../kernel_boot/boards/$BOARD_NAME/$DTB_FILENAME ./kernel_modules/
fi
tar -zcvf kernel_modules.tar.gz kernel_modules

scp kernel_modules.tar.gz root@192.168.10.122:

# scp $LINUX_KERNEL_IMAGE root@192.168.10.122:

scp populate_kernel_image_module_reboot.sh root@192.168.10.122:
