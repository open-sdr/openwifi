#!/bin/bash

# Author: Xianjun jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

print_usage () {
    echo "You must enter at least 2 arguments: \$XILINX_DIR ARCH_BIT(32 or 64)"
    echo "Further arguments (maximum 5) will be converted to #define argument in pre_def.h"
    echo " "
}

print_usage

if [ "$#" -lt 2 ]; then
    exit 1
fi

OPENWIFI_DIR=$(pwd)/../
XILINX_DIR=$1
ARCH_OPTION=$2

echo OPENWIFI_DIR $OPENWIFI_DIR
echo XILINX_DIR $XILINX_DIR
echo ARCH_OPTION $ARCH_OPTION

if [ -f "$OPENWIFI_DIR/LICENSE" ]; then
    echo "\$OPENWIFI_DIR is found!"
else
    echo "\$OPENWIFI_DIR is not correct. Please check!"
    exit 1
fi

if [ -d "$XILINX_DIR/Vitis" ]; then
    echo "\$XILINX_DIR is found!"
else
    echo "\$XILINX_DIR is not correct. Please check!"
    exit 1
fi

if [ "$ARCH_OPTION" != "32" ] && [ "$ARCH_OPTION" != "64" ]; then
    echo "\$ARCH_OPTION is not correct. Should be 32 or 64. Please check!"
    exit 1
else
    echo "\$ARCH_OPTION is valid!"
fi

XILINX_ENV_FILE=$XILINX_DIR/Vitis/2022.2/settings64.sh
echo "Expect env file $XILINX_ENV_FILE"

if [ -f "$XILINX_ENV_FILE" ]; then
    echo "$XILINX_ENV_FILE is found!"
else
    echo "$XILINX_ENV_FILE is not correct. Please check!"
    exit 1
fi

echo "#define USE_NEW_RX_INTERRUPT 1" > pre_def.h
if [[ -n $3 ]]; then
    DEFINE1=$3
    echo DEFINE1 $DEFINE1
    echo "#define $DEFINE1" >> pre_def.h
fi
if [[ -n $4 ]]; then
    DEFINE2=$4
    echo DEFINE2 $DEFINE2
    echo "#define $DEFINE2" >> pre_def.h
fi
if [[ -n $5 ]]; then
    DEFINE3=$5
    echo DEFINE3 $DEFINE3
    echo "#define $DEFINE3" >> pre_def.h
fi
if [[ -n $6 ]]; then
    DEFINE4=$6
    echo DEFINE4 $DEFINE4
    echo "#define $DEFINE4" >> pre_def.h
fi
if [[ -n $7 ]]; then
    DEFINE5=$7
    echo DEFINE5 $DEFINE5
    echo "#define $DEFINE5" >> pre_def.h
fi

source $XILINX_ENV_FILE

if [ "$ARCH_OPTION" == "64" ]; then
    LINUX_KERNEL_SRC_DIR=$OPENWIFI_DIR/adi-linux-64/
    ARCH="arm64"
    CROSS_COMPILE="aarch64-linux-gnu-"
else
    LINUX_KERNEL_SRC_DIR=$OPENWIFI_DIR/adi-linux/
    ARCH="arm"
    CROSS_COMPILE="arm-linux-gnueabihf-"
fi

# check if user entered the right path to analog device linux
if [ -d "$LINUX_KERNEL_SRC_DIR" ]; then
    echo "setup linux kernel path ${LINUX_KERNEL_SRC_DIR}"
else
    echo "Error: path to adi linux: ${LINUX_KERNEL_SRC_DIR} not found. Can not continue."
    exit 1
fi

set -x

home_dir=$(pwd)

cd $OPENWIFI_DIR/driver/
if git log -1; then
    echo "#define GIT_REV 0x"$(git log -1 --pretty=%h) > git_rev.h
else
    echo "#define GIT_REV 0xFFFFFFFF" > git_rev.h
fi
cd $OPENWIFI_DIR/driver/openofdm_tx
make KDIR=$LINUX_KERNEL_SRC_DIR ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
cd $OPENWIFI_DIR/driver/openofdm_rx
make KDIR=$LINUX_KERNEL_SRC_DIR ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
cd $OPENWIFI_DIR/driver/tx_intf
make KDIR=$LINUX_KERNEL_SRC_DIR ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
cd $OPENWIFI_DIR/driver/rx_intf
make KDIR=$LINUX_KERNEL_SRC_DIR ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
cd $OPENWIFI_DIR/driver/xpu
make KDIR=$LINUX_KERNEL_SRC_DIR ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
# cd $OPENWIFI_DIR/driver/ad9361
# make KDIR=$LINUX_KERNEL_SRC_DIR ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE

cd $OPENWIFI_DIR/driver/side_ch
./make_driver.sh $XILINX_DIR $ARCH_OPTION

cd $OPENWIFI_DIR/driver/
make KDIR=$LINUX_KERNEL_SRC_DIR ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE

cd $home_dir
