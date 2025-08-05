#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "You must enter 1 to apply new settings or 0 to restore default settings"
    exit 1
fi

set -x
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
set +x

if [ $1 == "0" ]; then 
  echo 0x15C 0x72 > direct_reg_access
  echo 0x106 0x72 > direct_reg_access
  echo 0x103 0x08 > direct_reg_access
  echo 0x101 0x0A > direct_reg_access
  echo 0x110 0x40 > direct_reg_access
  echo 0x115 0x00 > direct_reg_access
  echo 0x10A 0x58 > direct_reg_access
  echo "Applied default AGC settings"
elif [ $1 == "1" ]; then
  echo 0x15C 0x70 > direct_reg_access 
  echo 0x106 0x77 > direct_reg_access
  echo 0x103 0x1C > direct_reg_access
  echo 0x101 0x0C > direct_reg_access
  echo 0x110 0x48 > direct_reg_access
# DO NOT change 0x48 to 0x4A! Otherwise: did not acknowledge authentication response
  echo 0x114 0xb0 > direct_reg_access
#  0x30 is the original value for register 0x114
  echo 0x115 0x80 > direct_reg_access
  echo 0x10A 0x18 > direct_reg_access
  echo "Applied optimized AGC settings"
fi

# #Due to https://github.ugent.be/xjiao/openwifi/issues/148
echo 0x0fa 0x5 > direct_reg_access
echo 0x0fa 0xE5 > direct_reg_access
