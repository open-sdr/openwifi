#!/bin/bash

home_dir=$(pwd)

if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

set -x
# show
cat rx_data_pkt_num_total
cat rx_data_pkt_num_fail
cat rx_mgmt_pkt_num_total
cat rx_mgmt_pkt_num_fail
cat rx_ack_pkt_num_total
cat rx_ack_pkt_num_fail

cat rx_data_pkt_mcs_realtime
cat rx_data_pkt_fail_mcs_realtime
cat rx_mgmt_pkt_mcs_realtime
cat rx_mgmt_pkt_fail_mcs_realtime
cat rx_ack_pkt_mcs_realtime

cat rx_data_ok_agc_gain_value_realtime
cat rx_data_fail_agc_gain_value_realtime
cat rx_mgmt_ok_agc_gain_value_realtime
cat rx_mgmt_fail_agc_gain_value_realtime
cat rx_ack_ok_agc_gain_value_realtime

# clear
if [[ -n $1 ]]; then
  echo 0 > rx_data_pkt_num_total
  echo 0 > rx_data_pkt_num_fail
  echo 0 > rx_mgmt_pkt_num_total
  echo 0 > rx_mgmt_pkt_num_fail
  echo 0 > rx_ack_pkt_num_total
  echo 0 > rx_ack_pkt_num_fail

  echo 0 > rx_data_pkt_mcs_realtime
  echo 0 > rx_data_pkt_fail_mcs_realtime
  echo 0 > rx_mgmt_pkt_mcs_realtime
  echo 0 > rx_mgmt_pkt_fail_mcs_realtime
  echo 0 > rx_ack_pkt_mcs_realtime

  echo 0 > rx_data_ok_agc_gain_value_realtime
  echo 0 > rx_data_fail_agc_gain_value_realtime
  echo 0 > rx_mgmt_ok_agc_gain_value_realtime
  echo 0 > rx_mgmt_fail_agc_gain_value_realtime
  echo 0 > rx_ack_ok_agc_gain_value_realtime
fi
set +x

cd $home_dir

