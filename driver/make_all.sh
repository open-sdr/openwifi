#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "You must enter exactly 2 arguments: \$OPENWIFI_DIR \$XILINX_DIR"
    exit 1
fi

OPENWIFI_DIR=$1
XILINX_DIR=$2

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

adi_lnx_dir=$OPENWIFI_DIR/adi-linux/
xilinx_sdk_dir=$XILINX_DIR/SDK/2017.4/

# check if user entered the right path to SDK
if [ -d "$xilinx_sdk_dir" ]; then
  echo " setup sdk path ${xilinx_sdk_dir}"
  tmp=/settings64.sh
  sdk_setting="$xilinx_sdk_dir$tmp"
  source ${sdk_setting}
else
  echo "Error: sdk: ${xilinx_sdk_dir} not found. Can not continue."
  exit 1
fi

# check if user entered the right path to analog device linux
if [ -d "$adi_lnx_dir" ]; then
  echo " setup linux kernel path ${adi_lnx_dir}"
else
  echo "Error: path to adi linux: ${adi_lnx_dir} not found. Can not continue."
  exit 1
fi

home_dir=$(pwd)

#source ~/Xilinx/SDK/2017.4/settings64.sh
#set -x
cd $OPENWIFI_DIR/driver/
make KDIR=$adi_lnx_dir
cd $OPENWIFI_DIR/driver/openofdm_tx
make KDIR=$adi_lnx_dir
cd $OPENWIFI_DIR/driver/openofdm_rx
make KDIR=$adi_lnx_dir
cd $OPENWIFI_DIR/driver/tx_intf
make KDIR=$adi_lnx_dir
cd $OPENWIFI_DIR/driver/rx_intf
make KDIR=$adi_lnx_dir
cd $OPENWIFI_DIR/driver/xpu
make KDIR=$adi_lnx_dir
cd $OPENWIFI_DIR/driver/ad9361
make KDIR=$adi_lnx_dir
cd $OPENWIFI_DIR/driver/xilinx_dma
./make_xilinx_dma.sh $adi_lnx_dir $sdk_setting

cd $home_dir
