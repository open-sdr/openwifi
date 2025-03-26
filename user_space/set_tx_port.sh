#!/bin/bash

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

if [ "$#" -eq 1 ]; then
  if [ $1 == "0" ]; then 
	  echo 0x004 0x43 > direct_reg_access
	  status=$( cat direct_reg_access )
	  if [ $status == "0x43" ]; then
		  echo "Tx port B selected."
	  else
		  echo "WARNING: switching Tx port B unsuccessful"
	  fi 
  elif [ $1 == "1" ]; then
    echo 0x004 0x3 > direct_reg_access
	  status=$( cat direct_reg_access )
    if [ $status == "0x3" ]; then
		  echo "Tx port A selected."
	  else
		  echo "WARNING: switching Tx port A unsuccessful"
    fi 
  fi
elif [ "$#" -eq 0 ]; then
		echo "Reading status only. Enter 1 or 0 as argument to select port A or B."
    echo 0x004 > direct_reg_access
    status=$( cat direct_reg_access )
    if [ $status == "0x43" ]; then
		  echo "Tx port B is used"
    elif [ $status == "0x3" ]; then
      echo "Tx port A is used"
    else
      echo "WARNING Unrecognized value $status."
    fi  
else
  echo "Too many arguments, specify only one for selecting port A (1) or B (0)."
fi
