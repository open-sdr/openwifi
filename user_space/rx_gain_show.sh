#!/bin/bash

home_dir=$(pwd)

if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

set -x
cat rx_data_ok_agc_gain_value_realtime
cat rx_data_fail_agc_gain_value_realtime
cat rx_mgmt_ok_agc_gain_value_realtime
cat rx_mgmt_fail_agc_gain_value_realtime
cat rx_ack_ok_agc_gain_value_realtime
set +x

cd $home_dir

