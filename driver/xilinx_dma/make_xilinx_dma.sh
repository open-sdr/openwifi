#!/bin/bash
set -x

WORKDIR=$PWD
KDIR=$1
SUBMODULE=xilinx_dma

source $2
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

cp xilinx_dma.c $KDIR/drivers/dma/xilinx -rf
cd $KDIR
make $KDIR/drivers/dma/xilinx/$SUBMODULE.ko
cp $KDIR/drivers/dma/xilinx/$SUBMODULE.ko $WORKDIR -rf
cd $WORKDIR
ls $SUBMODULE.ko
