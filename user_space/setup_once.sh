!/bin/bash

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
  cp ./kernel_modules64/* ./kernel_modules/
  cp ./openwifi64/* ./openwifi/
    # cp ./kernel_modules64/* /lib/modules/$(uname -r)/
else
  cp ./kernel_modules32/* ./kernel_modules/
  cp ./openwifi32/* ./openwifi/
    # cp ./kernel_modules32/* /lib/modules/$(uname -r)/
fi

rm -rf /lib/modules/$(uname -r)
ln -s /root/kernel_modules /lib/modules/$(uname -r)
sync
depmod

cd ./openwifi/
cd sdrctl_src
make clean
make
cp sdrctl ../
cd ../side_ch_ctl_src/
gcc -o side_ch_ctl side_ch_ctl.c
cp side_ch_ctl ../
cd ../inject_80211/
make clean
make
cd ..
sync

# reboot now
