#!/bin/bash

# Author: Michael Mehari
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

# if [ "$#" -ne 1 ]; then
#     echo "You must enter exactly 1 arguments: \$OPENWIFI_DIR"
#     exit 1
# fi

OPENWIFI_DIR=$(pwd)/../

set -x

cd $OPENWIFI_DIR/user_space
wget http://w1.fi/releases/wpa_supplicant-2.1.tar.gz
tar xzvf wpa_supplicant-2.1.tar.gz
patch -d wpa_supplicant-2.1/src/drivers/ < driver_nl80211.patch
cd wpa_supplicant-2.1/wpa_supplicant/
cp defconfig .config
sed -i 's/#CONFIG_LIBNL32.*/CONFIG_LIBNL32=y/g' .config
make -j16
# sudo make install
cd ../../
rm -r wpa_supplicant-2.1/ wpa_supplicant-2.1.tar.gz
