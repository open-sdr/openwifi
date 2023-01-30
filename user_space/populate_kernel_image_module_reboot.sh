#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

set -ex

MACHINE_TYPE=`uname -m`

mkdir -p kernel_modules
rm -rf kernel_modules/*
tar -zxvf kernel_modules.tar.gz

if [ ${MACHINE_TYPE} == 'aarch64' ]; then
    IMAGE_FILENAME=Image
else
    IMAGE_FILENAME=uImage
fi

mv ./kernel_modules/ad9361_drv.ko ./openwifi/ -f || true
mv ./kernel_modules/adi_axi_hdmi.ko ./openwifi/ -f || true
mv ./kernel_modules/axidmatest.ko ./openwifi/ -f || true
mv ./kernel_modules/lcd.ko ./openwifi/ -f || true
mv ./kernel_modules/xilinx_dma.ko ./openwifi/ -f || true

rm -rf /lib/modules/$(uname -r)
ln -s /root/kernel_modules /lib/modules/$(uname -r)

depmod

umount /mnt || /bin/true
mount /dev/mmcblk0p1 /mnt
if test -f "./kernel_modules/$IMAGE_FILENAME"; then
    cp ./kernel_modules/$IMAGE_FILENAME /mnt/
fi
if test -f "./kernel_modules/BOOT.BIN"; then
    cp ./kernel_modules/BOOT.BIN /mnt/
fi
cd /mnt/
sync
cd ~
umount /mnt

reboot now
