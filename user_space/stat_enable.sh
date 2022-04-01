#!/bin/bash

home_dir=$(pwd)

if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

# set
if [[ -n $1 ]]; then
  echo $1 > stat_enable
else
  echo 1 > stat_enable
fi

# show
cat stat_enable

cd $home_dir

