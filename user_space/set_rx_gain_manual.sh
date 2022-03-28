#!/bin/sh

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

gain_dB=$1
if [ -z $gain_dB ]
then
  gain_dB=0
fi

home_dir=$(pwd)

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

echo manual > in_voltage0_gain_control_mode
cat in_voltage0_gain_control_mode

if [ $gain_dB -gt 0 ]; then
  echo $gain_dB > in_voltage0_hardwaregain
fi

cat in_voltage0_hardwaregain

cd $home_dir

set +x
