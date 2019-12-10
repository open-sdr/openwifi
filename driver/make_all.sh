#!/bin/bash
xilinx_sdk_dir=$1
adi_lnx_dir=$2\

if [ "$#" -ne 2 ]; then
    echo "You must enter exactly 2 command line arguments"
    echo "First argument is the path, second argument is the path to adi linux repository, please don't add slash at the end of the path"
    echo "Eg, ./make_all.sh /opt/Xilinx/SDK/2017.4 /home/gitfolder/linux"
    exit 1
fi

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



#source ~/Xilinx/SDK/2017.4/settings64.sh
#set -x
make KDIR=$adi_lnx_dir
cd openofdm_tx
make KDIR=$adi_lnx_dir
cd ../openofdm_rx
make KDIR=$adi_lnx_dir
cd ../tx_intf
make KDIR=$adi_lnx_dir
cd ../rx_intf
make KDIR=$adi_lnx_dir
cd ../xpu
make KDIR=$adi_lnx_dir
cd ../ad9361
make KDIR=$adi_lnx_dir
cd ../xilinx_dma
./make_xilinx_dma.sh $adi_lnx_dir $sdk_setting
cd ..


