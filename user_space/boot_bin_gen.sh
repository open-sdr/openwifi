#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -ne 3 ]; then
    echo "You must enter exactly 3 arguments: \$XILINX_DIR \$BOARD_NAME DIR_TO_filename.xsa"
    exit 1
fi

XILINX_DIR=$1
BOARD_NAME=$2
XSA_FILE=$3

OPENWIFI_DIR=$(pwd)/../

echo OPENWIFI_DIR $OPENWIFI_DIR
echo XSA_FILE $XSA_FILE

if [ -f "$OPENWIFI_DIR/LICENSE" ]; then
    echo "$OPENWIFI_DIR is found!"
else
    echo "$OPENWIFI_DIR is not correct. Please check!"
    exit 1
fi

XILINX_ENV_FILE=$XILINX_DIR/Vitis/2022.2/settings64.sh
echo "Expect env file $XILINX_ENV_FILE"

if [ -f "$XILINX_ENV_FILE" ]; then
    echo "$XILINX_ENV_FILE is found!"
else
    echo "$XILINX_ENV_FILE is not correct. Please check!"
    exit 1
fi

# if [ "$BOARD_NAME" != "antsdr" ] && [ "$BOARD_NAME" != "zc706_fmcs2" ] && [ "$BOARD_NAME" != "zc702_fmcs2" ] && [ "$BOARD_NAME" != "zed_fmcs2" ] && [ "$BOARD_NAME" != "adrv9361z7035" ] && [ "$BOARD_NAME" != "adrv9364z7020" ]; then
#     echo "$BOARD_NAME is not correct. Please check!"
#     exit 1
# else
#     echo "$BOARD_NAME is found!"
# fi

if [ -f "$XSA_FILE" ]; then
    echo "$XSA_FILE is found!"
else
    echo "$XSA_FILE is not found. Please check!"
    exit 1
fi

home_dir=$(pwd)

set -ex

source $XILINX_ENV_FILE

cd $OPENWIFI_DIR/kernel_boot

if [ "$BOARD_NAME" == "zcu102_fmcs2" ] || [ "$BOARD_NAME" == "zcu102_9371" ]; then
  ./build_zynqmp_boot_bin.sh $XSA_FILE boards/$BOARD_NAME/u-boot_xilinx_zynqmp_zcu102_revA.elf boards/$BOARD_NAME/bl31.elf
  ARCH="zynqmp"
  ARCH_BIT=64
elif [ "$BOARD_NAME" == "antsdr" ] || [ "$BOARD_NAME" == "antsdr_e200" ] || [ "$BOARD_NAME" == "e310v2" ] || [ "$BOARD_NAME" == "sdrpi" ] || [ "$BOARD_NAME" == "neptunesdr" ] || [ "$BOARD_NAME" == "zc706_fmcs2" ] || [ "$BOARD_NAME" == "zc702_fmcs2" ] || [ "$BOARD_NAME" == "zed_fmcs2" ] || [ "$BOARD_NAME" == "adrv9361z7035" ] || [ "$BOARD_NAME" == "adrv9364z7020" ]; then
  ./build_boot_bin.sh $XSA_FILE boards/$BOARD_NAME/u-boot.elf
  ARCH="zynq"
  ARCH_BIT=32
else
  echo "$BOARD_NAME is not correct. Please check!"
  cd $home_dir
  exit 1
fi

rm -rf build_boot_bin
rm -rf boards/$BOARD_NAME/output_boot_bin
mv output_boot_bin boards/$BOARD_NAME/

cd $home_dir

### Get basename of xsa and bit file
XSA_FILE_BASENAME="$(basename $XSA_FILE)"
XSA_FILE_BASENAME_WO_EXT="$(basename $XSA_FILE .xsa)"
BIT_FILE_BASENAME="$XSA_FILE_BASENAME_WO_EXT.bit"

# generate $BIT_FILE_BASENAME.bin for FPGA dynamic loading

echo "all:" > ./fpga_bit_to_bin.bif
echo "{" >> ./fpga_bit_to_bin.bif
echo "$BIT_FILE_BASENAME /* Bitstream file name */" >> ./fpga_bit_to_bin.bif
echo "}" >> ./fpga_bit_to_bin.bif

unzip -o $XSA_FILE
rm -rf ./$BIT_FILE_BASENAME.bin
bootgen -image fpga_bit_to_bin.bif -arch $ARCH -process_bitstream bin -w
ls ./$BIT_FILE_BASENAME.bin -al
