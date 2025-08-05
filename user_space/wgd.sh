#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2022 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

print_usage () {
  echo "usage:"
  echo "  Script for load (or download+load) different driver and FPGA img without rebooting"
  echo "  no  argument: Load .ko driver files and FPGA img (if system_top.bit.bin exist) in current dir with test_mode=0."
  echo "  1st argument: If it is a NUMBER, it will be assigned to test_mode. Then load everything from current dir."
  echo "  1st argument: If it is a string called \"remote\", it will download driver/FPGA and load everything."
  echo "  - 2nd argument (if exist) is the target directory name for downloading and reloading"
  echo "  - 3rd argument (if exist) is the value for test_mode"
  echo "  1st argument: neither NUMBER nor \"remote\" nor a .tar.gz file, it is regarded as a directory and load everything from it."
  echo "  - 2nd argument (if exist) is the value for test_mode"
  echo "  1st argument: a .tar.gz file, it will be unpacked then load from that unpacked directory"
  echo "  - 2nd argument (if exist) is the value for test_mode"
  echo " "
}

checkModule () {
  MODULE_input="$1"
  if lsmod | grep "$MODULE_input" &> /dev/null ; then
    echo "$MODULE_input is loaded!"
    return 0
  else
    echo "$MODULE_input is not loaded!"
    return 1
  fi
}

download_module () {
  MODULE_input="$1"
  TARGET_DIR_input="$2"
  mkdir -p $TARGET_DIR_input
  if [ "$MODULE_input" == "fpga" ]; then
    wget -O $TARGET_DIR_input/system_top.bit.bin ftp://192.168.10.1/user_space/system_top.bit.bin
  else
    if [ "$MODULE_input" == "sdr" ]; then
      wget -O $TARGET_DIR_input/$MODULE_input.ko ftp://192.168.10.1/driver/$MODULE_input.ko
    else
      wget -O $TARGET_DIR_input/$MODULE_input.ko ftp://192.168.10.1/driver/$MODULE_input/$MODULE_input.ko
    fi
  fi
  sync
}

insert_check_module () {
  TARGET_DIR_input="$1"
  MODULE_input="$2"
  sudo rmmod $MODULE_input
  if [[ -n $3 ]]; then
    (set -x; sudo insmod $TARGET_DIR_input/$MODULE_input.ko test_mode=$3)
  else
    (set -x; sudo insmod $TARGET_DIR_input/$MODULE_input.ko)
  fi

  checkModule $MODULE_input
  if [ $? -eq 1 ]; then
    exit 1
  fi
}

print_usage

# # now ad9361 driver is together with kernel. no need to load it.
# insmod ad9361_drv.ko

sudo insmod xilinx_dma.ko
# modprobe ad9361_drv
# modprobe xilinx_dma
sudo modprobe mac80211
sudo lsmod

TARGET_DIR=./
DOWNLOAD_FLAG=0
test_mode=0

if [[ -n $1 ]]; then
  re='^[0-9]+$'
  if ! [[ $1 =~ $re ]] ; then # not a number
    if [ "$1" == "remote" ]; then
      DOWNLOAD_FLAG=1
      if [[ -n $2 ]]; then
        TARGET_DIR=$2
      fi
      if [[ -n $3 ]]; then
        test_mode=$3
      fi
    else
      if [[ "$1" == *".tar.gz"* ]]; then
	set -x
        tar_gz_filename=$1
        TARGET_DIR=${tar_gz_filename%".tar.gz"}
        mkdir -p $TARGET_DIR
        rm -rf $TARGET_DIR/*
        tar -zxvf $1 -C $TARGET_DIR
        find $TARGET_DIR/ -name \*.ko -exec cp {} $TARGET_DIR/ \;
        find $TARGET_DIR/ -name \*.bit.bin -exec cp {} $TARGET_DIR/ \;
	set +x
      else
        TARGET_DIR=$1
      fi
      if [[ -n $2 ]]; then
        test_mode=$2
      fi
    fi
  else # is a number
    test_mode=$1
  fi
fi

echo TARGET_DIR $TARGET_DIR
echo DOWNLOAD_FLAG $DOWNLOAD_FLAG
echo test_mode $test_mode

#if ((($test_mode & 0x2) != 0)); then
  tx_offset_tuning_enable=0
#else
#  tx_offset_tuning_enable=1
#fi

echo tx_offset_tuning_enable $tx_offset_tuning_enable

if [ -d "$TARGET_DIR" ]; then
  echo "\$TARGET_DIR is found!"
else
  if [ $DOWNLOAD_FLAG -eq 0 ]; then
    echo "\$TARGET_DIR is not correct. Please check!"
    exit 1
  fi
fi

echo " "

killall hostapd
sudo service dhcpcd stop #dhcp client. it will get secondary ip for sdr0 which causes trouble
killall dhcpd 
killall wpa_supplicant
#service network-manager stop
sudo ifconfig sdr0 down

sudo rmmod sdr

if [ $DOWNLOAD_FLAG -eq 1 ]; then
  download_module fpga $TARGET_DIR
fi

if [ -f "$TARGET_DIR/system_top.bit.bin" ]; then
  sudo ./load_fpga_img.sh $TARGET_DIR/system_top.bit.bin
else
  echo $TARGET_DIR/system_top.bit.bin not found. Skip reloading FPGA.
  # sudo ./load_fpga_img.sh fjdo349ujtrueugjhj
fi

./rf_init_11n.sh

MODULE_ALL="tx_intf rx_intf openofdm_tx openofdm_rx xpu sdr"
for MODULE in $MODULE_ALL
do
  if [ $DOWNLOAD_FLAG -eq 1 ]; then
      download_module $MODULE $TARGET_DIR
  fi
  if [ "$MODULE" == "sdr" ]; then
    insert_check_module $TARGET_DIR $MODULE $test_mode
  else
    insert_check_module $TARGET_DIR $MODULE
  fi
done

# [ -e /tmp/check_calib_inf.pid ] && kill -0 $(</tmp/check_calib_inf.pid)
# ./check_calib_inf.sh

./agc_settings.sh 1

echo the end
# dmesg
# lsmod
