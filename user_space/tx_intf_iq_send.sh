#!/bin/bash

home_dir=$(pwd)

set -x
if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

echo 1 > tx_intf_iq_ctl
cat tx_intf_iq_ctl

cd $home_dir
