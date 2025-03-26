#!/bin/sh

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

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

echo fast_attack > in_voltage0_gain_control_mode
cat in_voltage0_gain_control_mode
cat in_voltage0_hardwaregain

if test -f "/sys/kernel/debug/iio/iio:device0/direct_reg_access"; then
  cd /sys/kernel/debug/iio/iio:device0/
else if test -f "/sys/kernel/debug/iio/iio:device1/direct_reg_access"; then
       cd /sys/kernel/debug/iio/iio:device1/
     else if test -f "/sys/kernel/debug/iio/iio:device2/direct_reg_access"; then
            cd /sys/kernel/debug/iio/iio:device2/
          else if test -f "/sys/kernel/debug/iio/iio:device3/direct_reg_access"; then
                 cd /sys/kernel/debug/iio/iio:device3/
               else if test -f "/sys/kernel/debug/iio/iio:device4/direct_reg_access"; then
                      cd /sys/kernel/debug/iio/iio:device4/
                    else
                      echo "Can not find direct_reg_access!"
                      echo "Check log to make sure ad9361 driver is loaded!"
                      exit 1
                    fi
               fi
          fi
     fi
fi

echo 0x0fa 0x5 > direct_reg_access
echo 0x0fa 0xE5 > direct_reg_access

cd $home_dir

set +x
