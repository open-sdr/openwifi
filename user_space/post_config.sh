#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

set -ex

MACHINE_TYPE=`uname -m`

# setup kernel module directory
if [ -d "/lib/modules/$(uname -r)" ]; then
    echo "/lib/modules/$(uname -r) already exists."
else
    if [ ${MACHINE_TYPE} == 'aarch64' ]; then
        ln -s /lib/modules/adi-linux-64 /lib/modules/$(uname -r)
    else
        ln -s /lib/modules/adi-linux /lib/modules/$(uname -r)
    fi
fi
depmod
modprobe mac80211

if [ ${MACHINE_TYPE} == 'aarch64' ]; then
    cp ~/openwifi/drv64/* ~/openwifi/ -rf
else
    cp ~/openwifi/drv32/* ~/openwifi/ -rf
fi

# add gateway (PC) for internet access
route add default gw 192.168.10.1 || true

sudo apt update

chmod +x *.sh

# build sdrctl
sudo apt-get -y install libnl-3-dev
sudo apt-get -y install libnl-genl-3-dev
cd sdrctl_src
make
cp sdrctl ../
cd ../side_ch_ctl_src/
gcc -o side_ch_ctl side_ch_ctl.c
cp side_ch_ctl ../
cd ..

# install and setup dhcp server
sudo apt-get -y install isc-dhcp-server
cp dhcpd.conf /etc/dhcp/dhcpd.conf

# install hostapd and other useful tools
sudo apt-get -y install hostapd
sudo apt-get -y install nano
sudo apt-get -y install tcpdump
sudo apt-get -y install webfs
sudo apt-get -y install iperf
sudo apt-get -y install iperf3
sudo apt-get -y install libpcap-dev
sudo apt-get -y install bridge-utils

cd ./inject_80211/
make

# change the root password to openwifi
cat /etc/passwd
sed -i 's/root:x:0:0:root:\/root:\/bin\/bash/root::0:0:root:\/root:\/bin\/bash/' /etc/passwd
sync
sleep 1
cat /etc/passwd
echo -e "openwifi\nopenwifi" | passwd
sync
sleep 1
cat /etc/passwd

