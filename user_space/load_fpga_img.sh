#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2022 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

# get fpga image file name
if [[ -n $1 ]]; then
  fpga_img_filename=$1
else
  fpga_img_filename=system_top.bit.bin
fi

fpga_type=$(tr '\000' '\n' < /proc/device-tree/compatible | head -n 1)
fpga_img_filename_core=${fpga_img_filename##*/}
echo "$fpga_type"
echo "$fpga_img_filename"
echo "$fpga_img_filename_core"

WAIT_RETRIES=${OPENWIFI_FPGA_WAIT_RETRIES:-50}
IIOD_WAS_RUNNING=0
IIOD_RESTART_METHOD=

stop_iiod () {
  if [ -x /etc/init.d/S99iiod ]; then
    /etc/init.d/S99iiod stop || return 1
    IIOD_RESTART_METHOD=buildroot
  elif command -v systemctl >/dev/null 2>&1 && \
       systemctl is-active --quiet iiod.service; then
    systemctl stop iiod.service || return 1
    IIOD_RESTART_METHOD=systemd
  else
    killall iiod 2>/dev/null || return 1
    IIOD_RESTART_METHOD=direct
  fi
}

start_iiod () {
  case "$IIOD_RESTART_METHOD" in
    buildroot)
      /etc/init.d/S99iiod start
      ;;
    systemd)
      systemctl start iiod.service
      ;;
    direct)
      command -v iiod >/dev/null 2>&1 || {
        echo "ERROR: iiod was running but is no longer installed" >&2
        return 1
      }
      iiod -D >/dev/null 2>&1 &
      ;;
    *)
      echo "ERROR: unknown iiod restart method" >&2
      return 1
      ;;
  esac
}

wait_for_path () {
  wait_path="$1"
  wait_count=0

  while [ ! -e "$wait_path" ]; do
    if [ "$wait_count" -ge "$WAIT_RETRIES" ]; then
      echo "ERROR: timeout waiting for $wait_path" >&2
      return 1
    fi
    echo "Waiting for $wait_path"
    sleep 0.2
    wait_count=$((wait_count + 1))
  done
}

bind_until_ready () {
  driver_path="$1"
  device_path="$2"
  device_name="$3"
  bind_count=0

  wait_for_path "$driver_path/bind" || return 1
  wait_for_path "$device_path/$device_name" || return 1

  while [ ! -e "$driver_path/$device_name" ]; do
    # A supplier rebind can leave a platform device present but unbound. In
    # that state waiting for the driver's symlink before writing bind creates
    # a deadlock, so actively retry the bind operation instead.
    { echo "$device_name" > "$driver_path/bind"; } 2>/dev/null || true
    if [ -e "$driver_path/$device_name" ]; then
      return 0
    fi
    if [ "$bind_count" -ge "$WAIT_RETRIES" ]; then
      echo "ERROR: unable to bind $device_name to ${driver_path##*/}" >&2
      return 1
    fi
    echo "Waiting for $device_name to bind to ${driver_path##*/}"
    sleep 0.2
    bind_count=$((bind_count + 1))
  done
}

unbind_if_bound () {
  driver_path="$1"
  device_name="$2"

  wait_for_path "$driver_path/unbind" || return 1
  if [ -e "$driver_path/$device_name" ]; then
    echo "$device_name" > "$driver_path/unbind" || return 1
  fi
}

if [ "$fpga_type" != "xlnx,zynq-7000" ]; then
  SPI_DEVNAME="spi1.0"
  DDS_DEVNAME="99024000.cf-ad9361-dds-core-lpc"
  ADC_DEVNAME="99020000.cf-ad9361-lpc"
else
  SPI_DEVNAME="spi0.0"
  DDS_DEVNAME="79024000.cf-ad9361-dds-core-lpc"
  ADC_DEVNAME="79020000.cf-ad9361-lpc"
fi

if [ -f "$fpga_img_filename" ]; then
  [ -s "$fpga_img_filename" ] || {
    echo "ERROR: FPGA image is empty: $fpga_img_filename" >&2
    exit 1
  }
  [ -d /sys/class/fpga_manager/fpga0 ] || {
    echo "ERROR: FPGA manager fpga0 is not available" >&2
    exit 1
  }
  [ -w /sys/class/fpga_manager/fpga0/firmware ] || {
    echo "ERROR: FPGA manager firmware attribute is not writable" >&2
    exit 1
  }
fi

set -x

ifconfig sdr0 down
rmmod sdr
# rmmod mac80211
# rmmod cfg80211
# rmmod ad9361_drv
# rmmod xilinx_dma.ko
rmmod openofdm_rx
rmmod openofdm_tx
rmmod rx_intf
rmmod tx_intf
rmmod xpu

sleep 1

if [ -f "$fpga_img_filename" ]; then
  # No user-space IIO request or bound PL consumer may access the AXI cores
  # while the programmable logic is being replaced.
  if pidof iiod >/dev/null 2>&1; then
    IIOD_WAS_RUNNING=1
    stop_iiod || exit 1
  fi
  unbind_if_bound \
    /sys/bus/platform/drivers/cf_axi_adc \
    "$ADC_DEVNAME" || exit 1
  unbind_if_bound \
    /sys/bus/platform/drivers/cf_axi_dds \
    "$DDS_DEVNAME" || exit 1
  unbind_if_bound \
    /sys/bus/spi/drivers/ad9361 \
    "$SPI_DEVNAME" || exit 1

  # Do not unbind/rebind the OpenWiFi AXI DMA platform devices. On E200 the
  # VDMA remove/probe path may access PL registers and lock the AXI bus after
  # a full PL reload. OpenWiFi consumers were removed above, so keeping these
  # topology-compatible DMA controllers bound but idle is safer.
  echo 0 > /sys/class/fpga_manager/fpga0/flags
  mkdir -p /lib/firmware
  cp "$fpga_img_filename" /lib/firmware/ -f
  if ! echo "$fpga_img_filename_core" > /sys/class/fpga_manager/fpga0/firmware; then
    echo "ERROR: FPGA manager failed to load $fpga_img_filename_core" >&2
    exit 1
  fi
fi

if true; then # only AD9361 RF need reset/re-connect currently

  # insmod ad9361_drv.ko
  # sleep 1

  bind_until_ready \
    /sys/bus/spi/drivers/ad9361 \
    /sys/bus/spi/devices \
    "$SPI_DEVNAME" || exit 1

  #while [ ! -d "/sys/bus/platform/drivers/cf_axi_dds/$DDS_DEVNAME" ]
  #do
  #   echo "Waiting for /sys/bus/platform/drivers/cf_axi_dds/$DDS_DEVNAME"
  #   sleep 0.2
  #done
  #cd /sys/bus/platform/drivers/cf_axi_dds/
  #echo $DDS_DEVNAME  > unbind
  #echo $DDS_DEVNAME  > bind

  # Rebinding AD9361 removes its IIO consumers through device links. DDS is
  # normally reprobed automatically, while ADC is not on some kernels. Bind
  # both explicitly when needed instead of waiting forever for a bound link.
  bind_until_ready \
    /sys/bus/platform/drivers/cf_axi_dds \
    /sys/bus/platform/devices \
    "$DDS_DEVNAME" || exit 1
  bind_until_ready \
    /sys/bus/platform/drivers/cf_axi_adc \
    /sys/bus/platform/devices \
    "$ADC_DEVNAME" || exit 1

fi

if [ "$IIOD_WAS_RUNNING" -eq 1 ]; then
  start_iiod || exit 1
fi

set +x
