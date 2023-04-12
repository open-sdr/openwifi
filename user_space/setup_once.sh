#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2023 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

set -ex

cd /root/

MACHINE_TYPE=`uname -m`

rm -rf kernel_modules
mkdir -p kernel_modules

# mkdir -p /lib/modules/$(uname -r)
# rm -rf /lib/modules/$(uname -r)
if [ ${MACHINE_TYPE} == 'aarch64' ]; then
  cp /root/kernel_modules64/* /root/kernel_modules/
  cp /root/openwifi64/* /root/openwifi/
    # cp ./kernel_modules64/* /lib/modules/$(uname -r)/
else
  cp /root/kernel_modules32/* /root/kernel_modules/
  cp /root/openwifi32/* /root/openwifi/
    # cp ./kernel_modules32/* /lib/modules/$(uname -r)/
fi

# Decide board name
DEVICE_TREE_MODEL_STRING=$(cat /proc/device-tree/model)
if [[ $DEVICE_TREE_MODEL_STRING == *"ADRV9361-Z7035"* ]]; then
  BOARD_NAME=adrv9361z7035
elif [[ $DEVICE_TREE_MODEL_STRING == *"ADRV9364-Z7020"* ]]; then
  BOARD_NAME=adrv9364z7020
elif [[ $DEVICE_TREE_MODEL_STRING == *"ANTSDR-E310V2"* ]]; then
  BOARD_NAME=e310v2
elif [[ $DEVICE_TREE_MODEL_STRING == *"ANTSDR-E310"* ]]; then
  BOARD_NAME=antsdr
elif [[ $DEVICE_TREE_MODEL_STRING == *"ANTSDR-E200"* ]]; then
  BOARD_NAME=antsdr_e200
elif [[ $DEVICE_TREE_MODEL_STRING == *"neptunesdr"* ]]; then
  BOARD_NAME=neptunesdr
elif [[ $DEVICE_TREE_MODEL_STRING == *"sdrpi"* ]]; then
  BOARD_NAME=sdrpi
elif [[ $DEVICE_TREE_MODEL_STRING == *"ZC702"* ]]; then
  BOARD_NAME=zc702_fmcs2
elif [[ $DEVICE_TREE_MODEL_STRING == *"ZC706"* ]]; then
  BOARD_NAME=zc706_fmcs2
elif [[ $DEVICE_TREE_MODEL_STRING == *"ZCU102"* ]]; then
  BOARD_NAME=zcu102_fmcs2
elif [[ $DEVICE_TREE_MODEL_STRING == *"ZED"* ]]; then
  BOARD_NAME=zed_fmcs2
else
  echo $DEVICE_TREE_MODEL_STRING " NOT recognized!"
  exit 1
fi

mv /root/kernel_modules/ad9361_drv.ko /root/openwifi/ -f || true
mv /root/kernel_modules/adi_axi_hdmi.ko /root/openwifi/ -f || true
mv /root/kernel_modules/axidmatest.ko /root/openwifi/ -f || true
mv /root/kernel_modules/lcd.ko /root/openwifi/ -f || true
mv /root/kernel_modules/xilinx_dma.ko /root/openwifi/ -f || true

rm -rf /lib/modules/$(uname -r)
ln -s /root/kernel_modules /lib/modules/$(uname -r)
sync
depmod

echo $BOARD_NAME
cp /root/openwifi_BOOT/$BOARD_NAME/system_top.bit.bin /root/openwifi/ -f || true

cd /root/openwifi/sdrctl_src
make clean
make
cp sdrctl /root/openwifi/
cd /root/openwifi/side_ch_ctl_src/
gcc -o side_ch_ctl side_ch_ctl.c
cp side_ch_ctl /root/openwifi/
cd /root/openwifi/inject_80211/
make clean
make
cd ..
sync

# reboot now
