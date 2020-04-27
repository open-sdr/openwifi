#!/bin/bash

if [ $# -ne 5 ]
  then
    echo "Please input slice_idx mac_addr cycle_period(us) start_time(us) end_time(us) as input parameter!"
    exit
fi

set -x #echo on

slice_idx=$1
mac_addr=$2
cycle_period=$3
start_time=$4
end_time=$5

echo $slice_idx
echo $mac_addr
echo $cycle_period
echo $start_time
echo $end_time

./sdrctl dev sdr0 set addr$slice_idx $mac_addr
./sdrctl dev sdr0 set slice_total$slice_idx $cycle_period
./sdrctl dev sdr0 set slice_start$slice_idx $start_time
./sdrctl dev sdr0 set slice_end$slice_idx $end_time
