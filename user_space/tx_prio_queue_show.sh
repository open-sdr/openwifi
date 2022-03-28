#!/bin/bash

home_dir=$(pwd)

if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

set -x
# show
cat tx_prio_queue

# clear
if [[ -n $1 ]]; then
  echo 0 > tx_prio_queue
fi
set +x

cd $home_dir

