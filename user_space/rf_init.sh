#!/bin/sh

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

home_dir=$(pwd)

if [ -z "$1" ]
then
  tx_offset_tuning_enable=1
else
  tx_offset_tuning_enable=0
fi
echo tx_offset_tuning_enable $tx_offset_tuning_enable

if [ $tx_offset_tuning_enable = "1" ]
then
  fir_filename="openwifi_ad9361_fir.ftr"
  tx_fir_enable=0
else
  fir_filename="openwifi_ad9361_fir_tx_0MHz.ftr"
  tx_fir_enable=1
fi

echo $fir_filename "tx_fir_enable" $tx_fir_enable

if test -f $fir_filename; then
  echo "Found" $fir_filename
else
  echo "Can not find" $fir_filename
  exit 1
fi

set -x
if test -f "/sys/bus/iio/devices/iio:device0/in_voltage_rf_bandwidth"; then
  cd /sys/bus/iio/devices/iio:device0/
else if test -f "/sys/bus/iio/devices/iio:device1/in_voltage_rf_bandwidth"; then
       cd /sys/bus/iio/devices/iio:device1/
     else if test -f "/sys/bus/iio/devices/iio:device2/in_voltage_rf_bandwidth"; then
            cd /sys/bus/iio/devices/iio:device2/
          else if test -f "/sys/bus/iio/devices/iio:device3/in_voltage_rf_bandwidth"; then
                 cd /sys/bus/iio/devices/iio:device3/
               else if test -f "/sys/bus/iio/devices/iio:device4/in_voltage_rf_bandwidth"; then
                      cd /sys/bus/iio/devices/iio:device4/
                    else
                      echo "Can not find in_voltage_rf_bandwidth!"
                      echo "Check log to make sure ad9361 driver is loaded!"
                      exit 1
                    fi
               fi
          fi
     fi
fi
set +x

echo 17500000 >  in_voltage_rf_bandwidth
sync
echo 37500000 >  out_voltage_rf_bandwidth
sync
echo 40000000 >  in_voltage_sampling_frequency
sync
echo 40000000 >  out_voltage_sampling_frequency
sync
sleep 1

echo 1000000000 >  out_altvoltage0_RX_LO_frequency
sync
echo 1000000000 >  out_altvoltage1_TX_LO_frequency
sync

cat $home_dir/$fir_filename > filter_fir_config
sync
sleep 0.5
echo 1 > in_voltage_filter_fir_en
echo $tx_fir_enable > out_voltage_filter_fir_en
cat filter_fir_config
cat in_voltage_filter_fir_en
cat out_voltage_filter_fir_en

echo "rx0 agc fast_attack"
#echo "rx0 agc manual"
cat in_voltage0_gain_control_mode
echo fast_attack > in_voltage0_gain_control_mode
#echo manual > in_voltage0_gain_control_mode
cat in_voltage0_gain_control_mode
sync

echo "rx1 agc fast_attack"
#echo "rx1 agc manual"
cat in_voltage1_gain_control_mode
echo fast_attack > in_voltage1_gain_control_mode
#echo manual > in_voltage1_gain_control_mode
cat in_voltage1_gain_control_mode
sync
sleep 1

cat in_voltage_sampling_frequency
cat in_voltage_rf_bandwidth
cat out_voltage_sampling_frequency
cat out_voltage_rf_bandwidth

echo "rssi"
cat in_voltage0_rssi
cat in_voltage1_rssi

# #  --------not needed maybe-------- # #
echo "rx0 gain to 70" # this set gain is gpio gain - 5dB (test with agc and read back gpio in driver)
cat in_voltage0_hardwaregain
echo 70 > in_voltage0_hardwaregain
cat in_voltage0_hardwaregain
sync

echo "rx1 gain to 70"
cat in_voltage1_hardwaregain
echo 70 > in_voltage1_hardwaregain
cat in_voltage1_hardwaregain
sync

echo "tx0 gain -89dB"
cat out_voltage0_hardwaregain
echo -89 > out_voltage0_hardwaregain
cat out_voltage0_hardwaregain
sync

echo "tx1 gain 0dB"
cat out_voltage1_hardwaregain
echo 0 > out_voltage1_hardwaregain
cat out_voltage1_hardwaregain
sync
# #  --------not needed maybe-------- # #

cd $home_dir

