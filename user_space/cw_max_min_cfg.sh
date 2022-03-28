#!/bin/bash

home_dir=$(pwd)

if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

set -x
#set
if [[ -n $1 ]]; then
  echo "$1" > cw_max_min_cfg
fi

# show
cat cw_max_min_cfg
set +x

cd $home_dir