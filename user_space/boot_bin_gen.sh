#!/bin/bash
if [ "$#" -ne 3 ]; then
    echo "You must enter exactly 3 arguments: \$OPENWIFI_DIR \$XILINX_DIR \$BOARD_NAME"
    exit 1
fi

OPENWIFI_DIR=$1
XILINX_DIR=$2
BOARD_NAME=$3

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

if [ "$BOARD_NAME" != "zc706_fmcs2" ] && [ "$BOARD_NAME" != "zc702_fmcs2" ] && [ "$BOARD_NAME" != "zed_fmcs2" ] && [ "$BOARD_NAME" != "adrv9361z7035" ] && [ "$BOARD_NAME" != "adrv9361z7035_fmc" ] && [ "$BOARD_NAME" != "adrv9364z7020" ]; then
    echo "\$BOARD_NAME is not correct. Please check!"
    exit 1
else
    echo "\$BOARD_NAME is found!"
fi

home_dir=$(pwd)

set -ex

# check if user entered the right path to SDK
if [ -d "$XILINX_DIR" ]; then
  source $XILINX_DIR/SDK/2017.4/settings64.sh
else
  echo "Error: XILINX_DIR: ${XILINX_DIR} not found. Can not continue."
  exit 1
fi

if [ -d "$OPENWIFI_DIR" ]; then
  cd $OPENWIFI_DIR/kernel_boot
else
  echo "Error: OPENWIFI_DIR: ${OPENWIFI_DIR} not found. Can not continue."
  exit 1
fi

./build_boot_bin.sh $BOARD_NAME

cd $home_dir
