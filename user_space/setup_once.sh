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

mv /root/kernel_modules/ad9361_drv.ko /root/openwifi/ -f || true
mv /root/kernel_modules/adi_axi_hdmi.ko /root/openwifi/ -f || true
mv /root/kernel_modules/axidmatest.ko /root/openwifi/ -f || true
mv /root/kernel_modules/lcd.ko /root/openwifi/ -f || true
mv /root/kernel_modules/xilinx_dma.ko /root/openwifi/ -f || true

rm -rf /lib/modules/$(uname -r)
ln -s /root/kernel_modules /lib/modules/$(uname -r)
sync
depmod

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
