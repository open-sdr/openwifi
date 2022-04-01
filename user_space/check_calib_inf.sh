#!/bin/bash

set -x
if test -f "/sys/kernel/debug/iio/iio:device0/direct_reg_access"; then
  device_path=/sys/kernel/debug/iio/iio:device0/
else if test -f "/sys/kernel/debug/iio/iio:device1/direct_reg_access"; then
       device_path=/sys/kernel/debug/iio/iio:device1/
     else if test -f "/sys/kernel/debug/iio/iio:device2/direct_reg_access"; then
            device_path=/sys/kernel/debug/iio/iio:device2/
          else if test -f "/sys/kernel/debug/iio/iio:device3/direct_reg_access"; then
                 device_path=/sys/kernel/debug/iio/iio:device3/
               else if test -f "/sys/kernel/debug/iio/iio:device4/direct_reg_access"; then
                      device_path=/sys/kernel/debug/iio/iio:device4/
                    else
                      echo "Check log to make sure ad9361 driver is loaded!"
                      exit 1
                    fi
               fi
          fi
     fi
fi
set +x

(bash -c 'echo $PPID' > /tmp/check_calib_inf.pid
while true; do
	echo 0x0A7 > ${device_path}direct_reg_access
	status=$( cat ${device_path}direct_reg_access )
	if [ $status == "0xFF" ]; then
		echo "WARNING: Tx Quadrature Calibration failed."
	fi
	sleep 5
done) &

