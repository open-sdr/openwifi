#!/bin/bash

set -ex

# setup kernel module directory
if [ -d "/lib/modules/$(uname -r)" ]; then
    echo "/lib/modules/$(uname -r) already exists."
else
    ln -s /lib/modules/openwifi /lib/modules/$(uname -r)
fi
depmod
modprobe mac80211

# add gateway (PC) for internet access
route add default gw 192.168.10.1 || true

# build sdrctl
sudo apt-get -y install libnl-3-dev
sudo apt-get -y install libnl-genl-3-dev
cd sdrctl_src
make
cp sdrctl ../
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

# change the password to openwifi
echo -e "openwifi\nopenwifi" | passwd
