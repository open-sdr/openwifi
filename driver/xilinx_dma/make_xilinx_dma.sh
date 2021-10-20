#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -ne 2 ]; then
    echo "You must enter exactly 2 arguments: \$XILINX_DIR \$ARCH(32 or 64)"
    exit 1
fi

WORKDIR=$PWD
OPENWIFI_DIR=$(pwd)/../../
XILINX_DIR=$1
ARCH_OPTION=$2

set -x

if [ -f "$OPENWIFI_DIR/LICENSE" ]; then
    echo "\$OPENWIFI_DIR is found!"
else
    echo "\$OPENWIFI_DIR is not correct. Please check!"
    exit 1
fi

if [ -d "$XILINX_DIR/SDK" ]; then
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

source $XILINX_DIR/SDK/2018.3/settings64.sh
if [ "$ARCH_OPTION" == "64" ]; then
    KDIR=$OPENWIFI_DIR/adi-linux-64/
    export ARCH=arm64
    export CROSS_COMPILE=aarch64-linux-gnu-
else
    KDIR=$OPENWIFI_DIR/adi-linux/
    export ARCH=arm
    export CROSS_COMPILE=arm-linux-gnueabihf-
fi

SUBMODULE=xilinx_dma

cp $KDIR/drivers/dma/xilinx/xilinx_dma.c $KDIR/drivers/dma/xilinx/xilinx_dma.c.bak
cp xilinx_dma.c $KDIR/drivers/dma/xilinx -rf
cd $KDIR
make $KDIR/drivers/dma/xilinx/$SUBMODULE.ko
cp $KDIR/drivers/dma/xilinx/$SUBMODULE.ko $WORKDIR -rf
# cp $KDIR/drivers/dma/xilinx/xilinx_dma.c.bak $KDIR/drivers/dma/xilinx/xilinx_dma.c
cd $WORKDIR
ls $SUBMODULE.ko
