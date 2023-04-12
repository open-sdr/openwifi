#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -ne 3 ]; then
    echo "You must enter exactly 3 arguments: \$XILINX_DIR \$BOARD_NAME DIR_TO_system_top.xsa"
    exit 1
fi

XILINX_DIR=$1
BOARD_NAME=$2
XSA_FILE=$3

OPENWIFI_DIR=$(pwd)/../

echo OPENWIFI_DIR $OPENWIFI_DIR
echo XSA_FILE $XSA_FILE

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

# if [ "$BOARD_NAME" != "antsdr" ] && [ "$BOARD_NAME" != "zc706_fmcs2" ] && [ "$BOARD_NAME" != "zc702_fmcs2" ] && [ "$BOARD_NAME" != "zed_fmcs2" ] && [ "$BOARD_NAME" != "adrv9361z7035" ] && [ "$BOARD_NAME" != "adrv9364z7020" ]; then
#     echo "\$BOARD_NAME is not correct. Please check!"
#     exit 1
# else
#     echo "\$BOARD_NAME is found!"
# fi

if [ -f "$XSA_FILE" ]; then
    echo "\$XSA_FILE is found!"
else
    echo "\$XSA_FILE is not found. Please check!"
    exit 1
fi

home_dir=$(pwd)

set -ex

source $XILINX_DIR/Vitis/2021.1/settings64.sh

cd $OPENWIFI_DIR/kernel_boot

if [ "$BOARD_NAME" == "zcu102_fmcs2" ] || [ "$BOARD_NAME" == "zcu102_9371" ]; then
  ./build_zynqmp_boot_bin.sh $XSA_FILE boards/$BOARD_NAME/u-boot_xilinx_zynqmp_zcu102_revA.elf boards/$BOARD_NAME/bl31.elf
  ARCH="zynqmp"
  ARCH_BIT=64
elif [ "$BOARD_NAME" == "antsdr" ] || [ "$BOARD_NAME" == "antsdr_e200" ] || [ "$BOARD_NAME" == "sdrpi" ] || [ "$BOARD_NAME" == "neptunesdr" ] || [ "$BOARD_NAME" == "zc706_fmcs2" ] || [ "$BOARD_NAME" == "zc702_fmcs2" ] || [ "$BOARD_NAME" == "zed_fmcs2" ] || [ "$BOARD_NAME" == "adrv9361z7035" ] || [ "$BOARD_NAME" == "adrv9364z7020" ]; then
  ./build_boot_bin.sh $XSA_FILE boards/$BOARD_NAME/u-boot.elf
  ARCH="zynq"
  ARCH_BIT=32
else
  echo "\$BOARD_NAME is not correct. Please check!"
  cd $home_dir
  exit 1
fi

rm -rf build_boot_bin
rm -rf boards/$BOARD_NAME/output_boot_bin
mv output_boot_bin boards/$BOARD_NAME/

cd $home_dir

# generate system_top.bit.bin for FPGA dynamic loading
unzip -o $XSA_FILE
rm -rf ./system_top.bit.bin
bootgen -image system_top.bif -arch $ARCH -process_bitstream bin -w
ls ./system_top.bit.bin -al
