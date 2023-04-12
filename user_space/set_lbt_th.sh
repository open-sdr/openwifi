#!/bin/bash

if [[ -n $1 ]]; then
  lbt_th=$1
else
  lbt_th=987654321 # no input
fi

set -x
#set
if [ $lbt_th -ne 987654321 ]; then
  ./sdrctl dev sdr0 set reg drv_xpu 0 $lbt_th
fi

# show
# ./sdrctl dev sdr0 get reg xpu 8 
./sdrctl dev sdr0 get reg drv_xpu 0
set +x
