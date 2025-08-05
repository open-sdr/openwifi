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
  re='^[0-9]+$'
  if ! [[ $1 =~ $re ]] ; then # not a number
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
  else # if it is a number, it means the target total number of packet for PER calculation
    num_received=$(cat rx_data_pkt_num_total)
    num_failed=$(cat rx_data_pkt_num_fail)
    num_correct=$(( $num_received - $num_failed ))
    num_total=$(( $1 ))
    PER_ENLARGE_FACTOR=10000
    num_correct_scale=$(( $num_correct * $PER_ENLARGE_FACTOR ))
    PCR=$(( $num_correct_scale / $num_total ))
    PER=$(( $PER_ENLARGE_FACTOR - $PCR ))
    echo PCR $PCR / $PER_ENLARGE_FACTOR
    echo PER $PER / $PER_ENLARGE_FACTOR
  fi
fi
set +x

cd $home_dir

