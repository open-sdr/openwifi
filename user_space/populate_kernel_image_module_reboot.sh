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

rm -rf /lib/modules/$(uname -r)
# setup kernel module directory
# if [ -d "/lib/modules/$(uname -r)" ]; then
#     echo "/lib/modules/$(uname -r) already exists."
# else
    # if [ ${MACHINE_TYPE} == 'aarch64' ]; then
    #     ln -s /root/kernel_modules /lib/modules/$(uname -r)
    # else
    #     ln -s /root/kernel_modules /lib/modules/$(uname -r)
    # fi
    ln -s /root/kernel_modules /lib/modules/$(uname -r)
# fi

depmod
# modprobe mac80211

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
