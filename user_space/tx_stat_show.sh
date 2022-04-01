#!/bin/bash

home_dir=$(pwd)

if test -d "/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr"; then
  cd /sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
else 
  cd /sys/devices/soc0/fpga-axi\@0/fpga-axi\@0\:sdr
fi

set -x
# show
cat tx_data_pkt_need_ack_num_total
cat tx_data_pkt_need_ack_num_total_fail
cat tx_data_pkt_need_ack_num_retx
cat tx_data_pkt_need_ack_num_retx_fail

cat tx_data_pkt_mcs_realtime
cat tx_data_pkt_fail_mcs_realtime

cat tx_mgmt_pkt_need_ack_num_total
cat tx_mgmt_pkt_need_ack_num_total_fail
cat tx_mgmt_pkt_need_ack_num_retx
cat tx_mgmt_pkt_need_ack_num_retx_fail

cat tx_mgmt_pkt_mcs_realtime
cat tx_mgmt_pkt_fail_mcs_realtime

# clear
if [[ -n $1 ]]; then
  echo 0 > tx_data_pkt_need_ack_num_total
  echo 0 > tx_data_pkt_need_ack_num_total_fail
  echo 0 > tx_data_pkt_need_ack_num_retx
  echo 0 > tx_data_pkt_need_ack_num_retx_fail

  echo 0 > tx_data_pkt_mcs_realtime
  echo 0 > tx_data_pkt_fail_mcs_realtime
  
  echo 0 > tx_mgmt_pkt_need_ack_num_total
  echo 0 > tx_mgmt_pkt_need_ack_num_total_fail
  echo 0 > tx_mgmt_pkt_need_ack_num_retx
  echo 0 > tx_mgmt_pkt_need_ack_num_retx_fail

  echo 0 > tx_mgmt_pkt_mcs_realtime
  echo 0 > tx_mgmt_pkt_fail_mcs_realtime
fi
set +x

cd $home_dir

