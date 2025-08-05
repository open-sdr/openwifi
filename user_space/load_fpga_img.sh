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

fpga_type=$(cat /proc/device-tree/compatible)
fpga_img_filename_core=${fpga_img_filename##*/}
echo $fpga_type
echo $fpga_img_filename
echo $fpga_img_filename_core

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
  echo 0 > /sys/class/fpga_manager/fpga0/flags
  mkdir -p /lib/firmware
  cp $fpga_img_filename /lib/firmware/ -rf
  echo $fpga_img_filename_core > /sys/class/fpga_manager/fpga0/firmware
fi

if true; then # only AD9361 RF need reset/re-connect currently

  # insmod ad9361_drv.ko
  # sleep 1

  if [ "$fpga_type" != "xlnx,zynq-7000" ]; then
    SPI_DEVNAME="spi1.0"
    DDS_DEVNAME="99024000.cf-ad9361-dds-core-lpc"
    ADC_DEVNAME="99020000.cf-ad9361-lpc"
  else
    SPI_DEVNAME="spi0.0"
    DDS_DEVNAME="79024000.cf-ad9361-dds-core-lpc"
    ADC_DEVNAME="79020000.cf-ad9361-lpc"
  fi

  while [ ! -d "/sys/bus/spi/drivers/ad9361/$SPI_DEVNAME" ]
  do
    echo "Waiting for /sys/bus/spi/drivers/ad9361/$SPI_DEVNAME"
    sleep 0.2
  done
  cd /sys/bus/spi/drivers/ad9361/
  echo $SPI_DEVNAME > unbind
  echo $SPI_DEVNAME > bind

  #while [ ! -d "/sys/bus/platform/drivers/cf_axi_dds/$DDS_DEVNAME" ]
  #do
  #   echo "Waiting for /sys/bus/platform/drivers/cf_axi_dds/$DDS_DEVNAME"
  #   sleep 0.2
  #done
  #cd /sys/bus/platform/drivers/cf_axi_dds/
  #echo $DDS_DEVNAME  > unbind
  #echo $DDS_DEVNAME  > bind

  while [ ! -d "/sys/bus/platform/drivers/cf_axi_adc/$ADC_DEVNAME" ]
  do
    echo "Waiting for /sys/bus/platform/drivers/cf_axi_adc/$ADC_DEVNAME"
    sleep 0.2
  done
  cd /sys/bus/platform/drivers/cf_axi_adc/
  echo $ADC_DEVNAME  > unbind
  echo $ADC_DEVNAME  > bind

fi

set +x
