#!/bin/bash

(bash -c 'echo $PPID' > /tmp/check_calib_inf.pid
while true; do
	echo 0x0A7 > /sys/kernel/debug/iio/iio:device2/direct_reg_access
	status=$( cat /sys/kernel/debug/iio/iio:device2/direct_reg_access )
	if [ $status == "0xFF" ]; then
		echo "WARNING: Tx Quadrature Calibration failed."
	fi
	sleep 5
done) &

