#!/bin/bash

# Reads RSSI in dB from RX1, let's call it "r".  
# Linear fit offset "o" depends on frequency (2.4GHz or 5GHz and FMCOMMS2/3).
# RSSI(dBm) = -r + o
# 2.4GHz(ch 6) FMCOMMS2: o = 16.74
# 2.4GHz(ch 6) FMCOMMS3: o = 17.44
# 5GHz (ch 44) FMCOMMS2: o = 25.41
# 5GHz (ch 44) FMCOMMS3: o = 24.58

home_dir=$(pwd)

#set -x
if test -f "/sys/bus/iio/devices/iio:device0/in_voltage0_rssi"; then
  cd /sys/bus/iio/devices/iio:device0/
else if test -f "/sys/bus/iio/devices/iio:device1/in_voltage0_rssi"; then
       cd /sys/bus/iio/devices/iio:device1/
     else if test -f "/sys/bus/iio/devices/iio:device2/in_voltage0_rssi"; then
            cd /sys/bus/iio/devices/iio:device2/
          else if test -f "/sys/bus/iio/devices/iio:device3/in_voltage0_rssi"; then
                 cd /sys/bus/iio/devices/iio:device3/
               else if test -f "/sys/bus/iio/devices/iio:device4/in_voltage0_rssi"; then
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
#set +x

if [ $# -lt 1 ]; then
  cat in_voltage0_rssi
else
  num_read=$1
  for ((i=0;i<$num_read;i++))
  do
    rssi_str=$(cat in_voltage0_rssi)
    echo "${rssi_str//dB}"
  done
fi
cd $home_dir
