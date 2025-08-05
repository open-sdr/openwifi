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
	  echo 0x051 0x10 > direct_reg_access
	  status=$( cat direct_reg_access )
	  if [ $status == "0x10" ]; then
		  echo "Tx LO turned off"
	  else
		  echo "WARNING: turning Tx LO off unsuccessful"
	  fi 
  elif [ $1 == "1" ]; then
    echo 0x051 0x0 > direct_reg_access
	  status=$( cat direct_reg_access )
    if [ $status == "0x0" ]; then
		  echo "Tx LO turned on"
	  else
		  echo "WARNING: turning Tx LO on unsuccessful"
    fi 
  fi
elif [ "$#" -eq 0 ]; then
		echo "Reading status only. Enter 1 or 0 as argument to set Tx LO on or off."
    echo 0x051 > direct_reg_access
    status=$( cat direct_reg_access )
    if [ $status == "0x10" ]; then
		  echo "Tx LO is off"
    elif [ $status == "0x0" ]; then
      echo "Tx LO is on"
    else
      echo "WARNING Unrecognized value $status."
    fi  
else
  echo "Too many arguments, specify only one for turning on (1) or off (0) the Tx LO."
fi
