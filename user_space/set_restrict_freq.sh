#!/bin/bash

home_dir=$(pwd)

if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

# set
if [[ -n $1 ]]; then
  echo $1 > restrict_freq_mhz
fi

# show
cat restrict_freq_mhz

cd $home_dir

