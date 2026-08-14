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
  echo "  OPENWIFI_RELOAD_FPGA=0/1 can be used to explicitly skip/force FPGA reload."
  echo " "
}

fpga_rf_chain_ready () {
  if [ -d /sys/bus/spi/drivers/ad9361/spi0.0 ]; then
    ADC_DEVNAME="79020000.cf-ad9361-lpc"
    DDS_DEVNAME="79024000.cf-ad9361-dds-core-lpc"
  elif [ -d /sys/bus/spi/drivers/ad9361/spi1.0 ]; then
    ADC_DEVNAME="99020000.cf-ad9361-lpc"
    DDS_DEVNAME="99024000.cf-ad9361-dds-core-lpc"
  else
    return 1
  fi

  [ -d "/sys/bus/platform/drivers/cf_axi_adc/$ADC_DEVNAME" ] &&
    [ -d "/sys/bus/platform/drivers/cf_axi_dds/$DDS_DEVNAME" ]
}

is_buildroot_system () {
  # This marker and init script are installed only by the OpenWiFi Buildroot
  # external tree. Keep the historical Kuiper/OpenWrt default of reloading the
  # local FPGA image, while Buildroot normally keeps the image loaded by U-Boot.
  [ -f /etc/openwifi-board ] && [ -x /etc/init.d/S02openwifi-board ]
}

ensure_debugfs () {
  if ! grep -qs '[[:space:]]/sys/kernel/debug[[:space:]]debugfs[[:space:]]' /proc/mounts; then
    mkdir -p /sys/kernel/debug
    if ! mount -t debugfs debugfs /sys/kernel/debug; then
      echo "ERROR: unable to mount debugfs at /sys/kernel/debug" >&2
      return 1
    fi
  fi
}

ensure_sdr_interface () {
  retry_count=0

  while [ "$retry_count" -lt 50 ]; do
    if [ -e /sys/class/net/sdr0 ]; then
      return 0
    fi

    for netdev_path in /sys/class/net/*; do
      [ -f "$netdev_path/address" ] || continue
      netdev_name=${netdev_path##*/}
      netdev_addr=$(cat "$netdev_path/address")
      case "$netdev_addr" in
        66:55:44:33:22:*)
          ip link set dev "$netdev_name" down 2>/dev/null || true
          if ip link set dev "$netdev_name" name sdr0; then
            echo "Renamed OpenWiFi interface $netdev_name to sdr0."
            return 0
          fi
          ;;
      esac
    done

    sleep 0.1
    retry_count=$((retry_count + 1))
  done

  echo "ERROR: OpenWiFi phy was registered but no 66:55:44:33:22:* network interface appeared." >&2
  iw dev >&2 || true
  return 1
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
  rmmod $MODULE_input
  if [[ -n $3 ]]; then
    (set -x; insmod $TARGET_DIR_input/$MODULE_input.ko test_mode=$3)
  else
    (set -x; insmod $TARGET_DIR_input/$MODULE_input.ko)
  fi

  checkModule $MODULE_input
  if [ $? -eq 1 ]; then
    exit 1
  fi
}

print_usage

if [ -f /etc/openwrt_release ]; then
  IS_OPENWRT="true"
else
  IS_OPENWRT="false"
fi


if [ "$IS_OPENWRT" = "true" ]; then
  # These modules are missing in OpenWrt but are present by default in ADI Kuiper
  echo "OpenWrt detected, installing ADI specific kernel modules..."
  insmod cf_axi_dds_drv.ko
  insmod cf_axi_adc.ko

  # For OpenWrt, OpenWiFi kernel modules are packed in the image under /lib/modules/$kernel_version
  kernel_version=$(uname -r)
  TARGET_DIR=/lib/modules/$kernel_version
else
  TARGET_DIR=.
fi

insmod ad9361_drv.ko
insmod xilinx_dma.ko
# modprobe ad9361_drv
# modprobe xilinx_dma
modprobe mac80211
lsmod

DOWNLOAD_FLAG=0
EXPLICIT_FPGA_IMAGE=0
test_mode=0

if [[ -n "$1" ]]; then
  re='^[0-9]+$'
  if ! [[ $1 =~ $re ]] ; then # not a number
    if [ "$1" == "remote" ]; then
      DOWNLOAD_FLAG=1
      EXPLICIT_FPGA_IMAGE=1
      if [[ -n $2 ]]; then
        TARGET_DIR=$2
      fi
      if [[ -n $3 ]]; then
        test_mode=$3
      fi
    else
      if [[ "$1" == *".tar.gz"* ]]; then
	EXPLICIT_FPGA_IMAGE=1
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
        EXPLICIT_FPGA_IMAGE=1
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
service dhcpcd stop #dhcp client. it will get secondary ip for sdr0 which causes trouble
killall dhcpd 
killall wpa_supplicant
#service network-manager stop
ifconfig sdr0 down

rmmod sdr

if [ $DOWNLOAD_FLAG -eq 1 ]; then
  download_module fpga $TARGET_DIR
fi

if [ -f "$TARGET_DIR/system_top.bit.bin" ]; then
  RELOAD_FPGA=0
  case "${OPENWIFI_RELOAD_FPGA:-auto}" in
    1|yes|true)
      RELOAD_FPGA=1
      ;;
    0|no|false)
      echo "OPENWIFI_RELOAD_FPGA=${OPENWIFI_RELOAD_FPGA}: skip FPGA reload."
      ;;
    auto)
      if [ "$EXPLICIT_FPGA_IMAGE" -eq 1 ]; then
        RELOAD_FPGA=1
      elif is_buildroot_system && fpga_rf_chain_ready; then
        echo "FPGA, AD9361, DDS and ADC are already initialized; skip duplicate FPGA reload."
        echo "Set OPENWIFI_RELOAD_FPGA=1 to force a dynamic reload."
      else
        echo "Use the historical FPGA reload behavior for this system."
        RELOAD_FPGA=1
      fi
      ;;
    *)
      echo "ERROR: OPENWIFI_RELOAD_FPGA must be auto, 0 or 1." >&2
      exit 1
      ;;
  esac

  if [ "$RELOAD_FPGA" -eq 1 ]; then
    ./load_fpga_img.sh "$TARGET_DIR/system_top.bit.bin" || exit 1
  fi
else
  echo "$TARGET_DIR/system_top.bit.bin not found. Skip reloading FPGA."
  # ./load_fpga_img.sh fjdo349ujtrueugjhj
fi

sleep 2
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

ensure_sdr_interface || exit 1
ensure_debugfs || exit 1
./agc_settings.sh 1 || exit 1

echo the end
# dmesg
# lsmod
