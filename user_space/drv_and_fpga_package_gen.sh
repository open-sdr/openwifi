  
#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2022 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -lt 3 ]; then
    echo "You have input $# arguments."
    echo "You must enter at least the first three 3 arguments: \$OPENWIFI_HW_IMG_DIR \$XILINX_DIR \$BOARD_NAME file_postfix"
    exit 1
fi

OPENWIFI_HW_IMG_DIR=$1
XILINX_DIR=$2
BOARD_NAME=$3

XILINX_ENV_FILE=$XILINX_DIR/Vitis/2022.2/settings64.sh
echo "Expect env file $XILINX_ENV_FILE"

if [ -f "$XILINX_ENV_FILE" ]; then
    echo "$XILINX_ENV_FILE is found!"
else
    echo "$XILINX_ENV_FILE is not correct. Please check!"
    exit 1
fi

if [ "$BOARD_NAME" != "neptunesdr" ] && [ "$BOARD_NAME" != "antsdr" ] && [ "$BOARD_NAME" != "antsdr_e200" ] && [ "$BOARD_NAME" != "e310v2" ]  && [ "$BOARD_NAME" != "sdrpi" ] && [ "$BOARD_NAME" != "zc706_fmcs2" ] && [ "$BOARD_NAME" != "zc702_fmcs2" ] && [ "$BOARD_NAME" != "zed_fmcs2" ] && [ "$BOARD_NAME" != "adrv9361z7035" ] && [ "$BOARD_NAME" != "adrv9364z7020" ] && [ "$BOARD_NAME" != "zcu102_fmcs2" ] && [ "$BOARD_NAME" != "zcu102_9371" ]; then
    echo "$BOARD_NAME is not correct. Please check!"
    exit 1
else
    echo "$BOARD_NAME is found!"
fi

if [ -d "$OPENWIFI_HW_IMG_DIR/boards/$BOARD_NAME" ]; then
    echo "$OPENWIFI_HW_IMG_DIR is found!"
else
    echo "$OPENWIFI_HW_IMG_DIR is not correct. Please check!"
    exit 1
fi

# uncompress the system.hdf and system_top.bit for use
mkdir -p hdf_and_bit
rm hdf_and_bit/* -rf
unzip $OPENWIFI_HW_IMG_DIR/boards/$BOARD_NAME/sdk/system_top.xsa -d ./hdf_and_bit
# cp ./hdf_and_bit/$BOARD_NAME/sdk/system_top_hw_platform_0/system.hdf $OPENWIFI_HW_IMG_DIR/boards/$BOARD_NAME/sdk/system_top_hw_platform_0/ -rf
# cp ./hdf_and_bit/system_top.bit $OPENWIFI_HW_IMG_DIR/boards/$BOARD_NAME/sdk/system_top_hw_platform_0/ -rf

# BIT_FILENAME=$OPENWIFI_HW_IMG_DIR/boards/$BOARD_NAME/sdk/system_top_hw_platform_0/system_top.bit
BIT_FILENAME=./hdf_and_bit/system_top.bit

if [ -f "$BIT_FILENAME" ]; then
    echo "$BIT_FILENAME is found!"
else
    echo "$BIT_FILENAME does NOT exist. Please check!"
    exit 1
fi

if [ "$BOARD_NAME" == "zcu102_fmcs2" ] || [ "$BOARD_NAME" == "zcu102_9371" ]; then
    ARCH="zynqmp"
    ARCH_BIT=64
else
    ARCH="zynq"
    ARCH_BIT=32
fi

# FINAL_BIT_FILENAME=$BOARD_NAME\_system_top_reload.bit.bin

set -x

source $XILINX_ENV_FILE

cp $BIT_FILENAME ./
bootgen -image system_top.bif -arch $ARCH -process_bitstream bin -w

# cp system_top_reload.bit.bin ./$FINAL_BIT_FILENAME

cd ../driver
make clean
./make_all.sh $XILINX_DIR $ARCH_BIT
cd ../user_space
mkdir -p drv_and_fpga
rm -rf drv_and_fpga/*
cp system_top.bit.bin ../driver/side_ch/side_ch.ko ../driver/tx_intf/tx_intf.ko ../driver/rx_intf/rx_intf.ko ../driver/openofdm_tx/openofdm_tx.ko ../driver/openofdm_rx/openofdm_rx.ko  ../driver/xpu/xpu.ko ../driver/sdr.ko ./drv_and_fpga -f
cp $OPENWIFI_HW_IMG_DIR/boards/$BOARD_NAME/sdk/git_info.txt ./drv_and_fpga -f
# Add driver git info
echo " " >> ./drv_and_fpga//git_info.txt
echo "openwifi-git-branch" >> ./drv_and_fpga//git_info.txt
git branch >> ./drv_and_fpga//git_info.txt
echo " " >> ./drv_and_fpga//git_info.txt
echo "openwifi-git-commit" >> ./drv_and_fpga//git_info.txt
git log -3 >> ./drv_and_fpga//git_info.txt
echo " " >> ./drv_and_fpga//git_info.txt

tar -cvf ./drv_and_fpga/driver.tar $(git ls-files ../driver/)

# dir_save=$(pwd)

# cd $OPENWIFI_HW_DIR/ip/
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-ip-root.tar $(git ls-files ./ | grep -v -E "/|openofdm_rx")
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-ip-xpu.tar $(git ls-files ./xpu)
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-ip-tx_intf.tar $(git ls-files ./tx_intf)
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-ip-rx_intf.tar $(git ls-files ./rx_intf)
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-ip-openofdm_tx.tar $(git ls-files ./openofdm_tx)
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-ip-side_ch.tar $(git ls-files ./side_ch)

# cd ../boards
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-boards-root.tar $(git ls-files ./ | grep -v "/")
# cd ./$BOARD_NAME
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-boards-$BOARD_NAME-root.tar $(git ls-files ./ | grep -v "/")
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-boards-$BOARD_NAME-src.tar $(git ls-files ./src)
# tar -cvf $dir_save/drv_and_fpga/openwifi-hw-boards-$BOARD_NAME-ip_repo.tar ip_repo

# cd $dir_save
# # tar -cvf drv_and_fpga.tar system_top.bit.bin tx_intf.ko rx_intf.ko openofdm_tx.ko openofdm_rx.ko xpu.ko sdr.ko git_info.txt

if [ "$#" == 4 ]; then
    POSTFIX="_${4}"
else
    POSTFIX=""
fi
tar -zcvf drv_and_fpga$POSTFIX.tar.gz drv_and_fpga

set +x
