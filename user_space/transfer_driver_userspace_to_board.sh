  
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

# if [ "$#" -ne 2 ]; then
#     echo "You have input $# arguments."
#     echo "You must enter \$DIR_TO_ADI_LINUX_KERNEL and ARCH_BIT(32 or 64) as argument"
#     exit 1
# fi

# DIR_TO_ADI_LINUX_KERNEL=$1
# ARCH_OPTION=$2

# if [ "$ARCH_OPTION" == "64" ]; then
#     LINUX_KERNEL_IMAGE=$DIR_TO_ADI_LINUX_KERNEL/arch/arm64/boot/Image
# else
#     LINUX_KERNEL_IMAGE=$DIR_TO_ADI_LINUX_KERNEL/arch/arm/boot/uImage
# fi

mkdir -p openwifi
rm -rf ./openwifi/*
find ../driver/ -name \*.ko -exec cp {} ./openwifi/ \;

tar -zcvf openwifi.tar.gz openwifi

scp openwifi.tar.gz root@192.168.10.122:
scp populate_driver_userspace.sh root@192.168.10.122:
