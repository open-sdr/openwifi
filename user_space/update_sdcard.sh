  
#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

# Only put BOOT partition (BOOT.BIN devicetree kernel) and kernel modules drivers on the SD card, but not populate them

if [ "$#" -lt 3 ]; then
    echo "You have input $# arguments."
    echo "You must enter exactly 3 arguments: \$OPENWIFI_HW_IMG_DIR \$XILINX_DIR \$SDCARD_DIR"
    exit 1
fi

SKIP_KERNEL_BUILD=0
SKIP_BOOT=0
SKIP_rootfs=0
if [ "$#" -gt 3 ]; then
    SKIP_KERNEL_BUILD=$(( ($4 >> 0) & 1 ))
    SKIP_BOOT=$(( ($4 >> 1) & 1 ))
    SKIP_rootfs=$(( ($4 >> 2) & 1 ))
    echo $4
    echo SKIP_KERNEL_BUILD $SKIP_KERNEL_BUILD
    echo SKIP_BOOT $SKIP_BOOT
    echo SKIP_rootfs $SKIP_rootfs
fi

BOARD_NAME_ALL="sdrpi antsdr antsdr_e200 e310v2 zc706_fmcs2 zed_fmcs2 zc702_fmcs2 adrv9361z7035 adrv9364z7020 zcu102_fmcs2 neptunesdr"
if [ "$#" -gt 4 ]; then
    BOARD_NAME_ALL=$5
    echo BOARD_NAME_ALL $BOARD_NAME_ALL
fi

OPENWIFI_HW_IMG_DIR=$1
XILINX_DIR=$2
SDCARD_DIR=$3

OPENWIFI_DIR=$(pwd)/../

echo OPENWIFI_DIR $OPENWIFI_DIR
echo OPENWIFI_HW_IMG_DIR $OPENWIFI_HW_IMG_DIR

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

if [ -d "$OPENWIFI_HW_IMG_DIR/boards/" ]; then
    echo "\$OPENWIFI_HW_IMG_DIR is found!"
else
    echo "\$OPENWIFI_HW_IMG_DIR is not correct. Please check!"
    exit 1
fi

# detect SD card mounting status
if [ -d "$SDCARD_DIR/BOOT/" ]; then
    echo "$SDCARD_DIR/BOOT/"
    sudo rm -f $SDCARD_DIR/BOOT/README.txt
    # to save some space
    sudo rm -rf $SDCARD_DIR/BOOT/socfpga_*
    sudo rm -rf $SDCARD_DIR/BOOT/versal-*
else
    echo "$SDCARD_DIR/BOOT/ does not exist!"
    exit 1
fi

if [ -d "$SDCARD_DIR/rootfs/" ]; then
    echo "$SDCARD_DIR/rootfs/"
else
    echo "$SDCARD_DIR/rootfs/ does not exist!"
    exit 1
fi

sudo true

home_dir=$(pwd)

set -x

LINUX_KERNEL_SRC_DIR_NAME32=adi-linux
LINUX_KERNEL_SRC_DIR_NAME64=adi-linux-64

if [ "$SKIP_KERNEL_BUILD" == "0" ]; then
  cd $OPENWIFI_DIR/user_space/
  ./prepare_kernel.sh $XILINX_DIR 32
  sudo true
  ./prepare_kernel.sh $XILINX_DIR 64
  sudo true
fi

if [ "$SKIP_BOOT" == "0" ]; then
  sudo rm -rf $SDCARD_DIR/BOOT/openwifi/
  sudo mkdir -p $SDCARD_DIR/BOOT/openwifi
  for BOARD_NAME_TMP in $BOARD_NAME_ALL
  do
      if [ "$BOARD_NAME_TMP" == "zcu102_fmcs2" ] || [ "$BOARD_NAME_TMP" == "zcu102_9371" ]; then
          dtb_filename_tmp="system.dtb"
          dts_filename_tmp="system.dts"
      else
          dtb_filename_tmp="devicetree.dtb"
          dts_filename_tmp="devicetree.dts"
          kernel_img_filename_tmp=$OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME32/arch/arm/boot/uImage
      fi
      ./boot_bin_gen.sh $XILINX_DIR $BOARD_NAME_TMP $OPENWIFI_HW_IMG_DIR/boards/$BOARD_NAME_TMP/sdk/system_top.xsa
      echo $dtb_filename_tmp
      echo $dts_filename_tmp

      dtc -I dts -O dtb -o $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME_TMP/$dtb_filename_tmp $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME_TMP/$dts_filename_tmp
      sudo mkdir -p $SDCARD_DIR/BOOT/openwifi/$BOARD_NAME_TMP
      sudo cp $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME_TMP/$dtb_filename_tmp $SDCARD_DIR/BOOT/openwifi/$BOARD_NAME_TMP/
      sudo cp $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME_TMP/output_boot_bin/BOOT.BIN $SDCARD_DIR/BOOT/openwifi/$BOARD_NAME_TMP/
      sudo cp ./system_top.bit.bin $SDCARD_DIR/BOOT/openwifi/$BOARD_NAME_TMP/
      sudo true
  done

  kernel_img_filename_tmp=$OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME64/arch/arm64/boot/Image
  sudo cp $kernel_img_filename_tmp $SDCARD_DIR/BOOT/
  kernel_img_filename_tmp=$OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME32/arch/arm/boot/uImage
  sudo cp $kernel_img_filename_tmp $SDCARD_DIR/BOOT/
fi

if [ "$SKIP_rootfs" == "0" ]; then
  sudo rm -rf $SDCARD_DIR/rootfs/root/openwifi/
  sudo mkdir -p $SDCARD_DIR/rootfs/root/openwifi

  saved_dir=$(pwd)
  cd $OPENWIFI_DIR/user_space/
  git clean -dxf ./
  cd $saved_dir
  sudo cp $OPENWIFI_DIR/user_space/* $SDCARD_DIR/rootfs/root/openwifi/ -rf
  sudo mv $SDCARD_DIR/rootfs/root/openwifi/system_top.bit.bin $SDCARD_DIR/rootfs/root/openwifi/system_top.bit.bin.bak
  sudo wget -P $SDCARD_DIR/rootfs/root/openwifi/webserver/ https://github.com/open-sdr/openwifi-hw-img/raw/master/openwifi-low-aac.mp4

  sudo rm -rf $SDCARD_DIR/rootfs/root/openwifi_BOOT/
  sudo mkdir -p $SDCARD_DIR/rootfs/root/openwifi_BOOT
  sudo cp $SDCARD_DIR/BOOT/openwifi/* $SDCARD_DIR/rootfs/root/openwifi_BOOT/ -rf

  ARCH_OPTION_ALL="32 64"
  for ARCH_OPTION_TMP in $ARCH_OPTION_ALL
  do
    # build openwifi driver
    saved_dir=$(pwd)
    cd $OPENWIFI_DIR/driver/
    git clean -dxf ./
    sync
    ./make_all.sh $XILINX_DIR $ARCH_OPTION_TMP
    cd $saved_dir

    # Copy files to SD card rootfs partition
    sudo rm -rf $SDCARD_DIR/rootfs/root/openwifi$ARCH_OPTION_TMP/
    sudo mkdir -p $SDCARD_DIR/rootfs/root/openwifi$ARCH_OPTION_TMP
    sudo find $OPENWIFI_DIR/driver/ -name \*.ko -exec cp {} $SDCARD_DIR/rootfs/root/openwifi$ARCH_OPTION_TMP \;

    sudo rm -rf $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
    sudo mkdir -p $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP

    if [ "$ARCH_OPTION_TMP" == "32" ]; then
      sudo find $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME32 -name \*.ko -exec cp {} $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/ \;
      sudo cp $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME32/Module.symvers $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
      sudo cp $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME32/modules.builtin $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
      sudo cp $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME32/modules.builtin.modinfo $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
      sudo cp $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME32/modules.order $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
    else
      sudo find $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME64 -name \*.ko -exec cp {} $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/ \;
      sudo cp $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME64/Module.symvers $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
      sudo cp $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME64/modules.builtin $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
      sudo cp $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME64/modules.builtin.modinfo $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
      sudo cp $OPENWIFI_DIR/$LINUX_KERNEL_SRC_DIR_NAME64/modules.order $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/
    fi

    sudo rm -rf $SDCARD_DIR/rootfs/lib/modules/*dirty*
    sudo rm -rf $SDCARD_DIR/rootfs/root/kernel_modules

    # sudo rm $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/axidmatest.ko -f
    # sudo rm $SDCARD_DIR/rootfs/root/kernel_modules$ARCH_OPTION_TMP/adi_axi_hdmi.ko -f
  done

fi

cd $SDCARD_DIR/BOOT
sync
cd $SDCARD_DIR/rootfs
sync

cd $home_dir

sudo umount $SDCARD_DIR/BOOT/
sudo umount $SDCARD_DIR/rootfs/
