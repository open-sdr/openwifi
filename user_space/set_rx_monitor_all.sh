#!/bin/bash

home_dir=$(pwd)

if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

set -x
# set
if [[ -n $1 ]]; then
  echo $1 > rx_monitor_all
else
  echo 1 > rx_monitor_all
fi

# show
cat rx_monitor_all
set +x

cd $home_dir

