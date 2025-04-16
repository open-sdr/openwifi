  
#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

# ATTENTION! You need Vitis, NOT Vitis_HLS, installed

# if [ "$#" -ne 1 ]; then
#     echo "You must enter 1 arguments: ARCH_BIT(32 or 64)"
#     exit 1
# fi

# OPENWIFI_DIR=$(pwd)/../
# ARCH_OPTION=$1

if [ "$#" -ne 2 ]; then
    echo "You must enter 2 arguments: \$XILINX_DIR ARCH_BIT(32 or 64)"
    exit 1
fi

OPENWIFI_DIR=$(pwd)/../
XILINX_DIR=$1
ARCH_OPTION=$2

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

if [ "$ARCH_OPTION" == "64" ]; then
  LINUX_KERNEL_SRC_DIR_NAME=adi-linux-64
  LINUX_KERNEL_CONFIG_FILE=$OPENWIFI_DIR/kernel_boot/kernel_config_zynqmp
  ARCH_NAME="arm64"
  CROSS_COMPILE_NAME="aarch64-linux-gnu-"
  IMAGE_TYPE=Image
else
  LINUX_KERNEL_SRC_DIR_NAME=adi-linux
  LINUX_KERNEL_CONFIG_FILE=$OPENWIFI_DIR/kernel_boot/kernel_config
  ARCH_NAME="arm"
  CROSS_COMPILE_NAME="arm-linux-gnueabihf-"
  IMAGE_TYPE=uImage
fi

home_dir=$(pwd)

set -x

cd $OPENWIFI_DIR/
git submodule init $LINUX_KERNEL_SRC_DIR_NAME
cd $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME
git reset --hard
cd $OPENWIFI_DIR/
git submodule update $LINUX_KERNEL_SRC_DIR_NAME
cd $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME
if false; then
  echo "Reserve for future"
else
  git fetch
  git checkout 2022_R2
  git pull origin 2022_R2
  # git reset --hard 2022_R2
  git reset --hard c2f371e014f0704be4db02e5014c51ae99477c13 # save this commit for tsn
fi

source $XILINX_ENV_FILE
export ARCH=$ARCH_NAME
export CROSS_COMPILE=$CROSS_COMPILE_NAME

if false; then
  echo "Reserve for future"
else
  # if [ "$ARCH_OPTION" == "64" ]; then
    cp $LINUX_KERNEL_CONFIG_FILE ./.config
    # cp $OPENWIFI_DIR/driver/ad9361/ad9361.c $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME/drivers/iio/adc/ad9361.c -rf
    # cp $OPENWIFI_DIR/driver/ad9361/ad9361_conv.c $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME/drivers/iio/adc/ad9361_conv.c -rf
    git apply ../kernel_boot/axi_hdmi_crtc.patch
    git apply ../kernel_boot/ad9361.patch
    git apply ../kernel_boot/ad9361_private.patch
    git apply ../kernel_boot/ad9361_conv.patch
    # #Ignore warning in mac80211 -- NOT necessary for 2022_R2 kernel!
    # sed -i '3692 s/^/\/\//' ../$LINUX_KERNEL_SRC_DIR_NAME/net/mac80211/util.c
  # else
    # make zynq_xcomm_adv7511_defconfig
  # fi

  make oldconfig
  # make adi_zynqmp_defconfig
  make prepare && make modules_prepare

  # if [ "$#" -gt 2 ]; then
  make -j12 $IMAGE_TYPE UIMAGE_LOADADDR=0x8000
  make modules
  # fi
fi

cd $home_dir
