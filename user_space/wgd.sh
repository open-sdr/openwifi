#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

checkModule () {
  MODULE="$1"
  if lsmod | grep "$MODULE" &> /dev/null ; then
    echo "$MODULE is loaded!"
    return 0
  else
    echo "$MODULE is not loaded!"
    return 1
  fi
}

num_arg=$#
echo "num_arg " $num_arg

for input_var in "$@"
do
    last_input=$input_var
#    echo $last_input
done

if [ -z $last_input ]
then
	last_input="unset"
  test_mode=0
else
  if [ $1 -eq 1 ]
  then
    test_mode=1
  elif [ $1 -eq 2 ]
  then
    test_mode=2
  elif [ $1 -eq 3 ]
  then
    test_mode=3
  elif [ $1 -eq 4 ]
  then
    test_mode=4
  elif [ $1 -eq 5 ]
  then
    test_mode=5
  elif [ $1 -eq 6 ]
  then
    test_mode=6
  elif [ $1 -eq 7 ]
  then
    test_mode=7
  else
    test_mode=0
  fi
fi

#if ((($test_mode & 0x2) != 0)); then
  tx_offset_tuning_enable=0
#else
#  tx_offset_tuning_enable=1
#fi

echo last_input $last_input
echo test_mode $test_mode
echo tx_offset_tuning_enable $tx_offset_tuning_enable

service network-manager stop

depmod
modprobe mac80211
lsmod
# dmesg -c

PROG=sdr
rmmod $PROG


# mv ad9361 driver to local folder, to prevent booting issue
if [ -f /lib/modules/$(uname -r)/ad9361_drv.ko ]; then
   mv /lib/modules/$(uname -r)/ad9361_drv.ko .
fi
SUBMODULE=ad9361_drv
rmmod $SUBMODULE
insmod $SUBMODULE.ko

echo check $SUBMODULE module is loaded or not
checkModule $SUBMODULE
if [ $? -eq 1 ]
then
  return
fi
sleep 1
lsmod

# mv xilinx dma driver to local folder, to prevent booting issue
if [ -f /lib/modules/$(uname -r)/xilinx_dma.ko ]; then
   mv /lib/modules/$(uname -r)/xilinx_dma.ko .
fi
SUBMODULE=xilinx_dma
rmmod $SUBMODULE
insmod $SUBMODULE.ko

#sleep 1

echo check $SUBMODULE module is loaded or not
checkModule $SUBMODULE
if [ $? -eq 1 ]
then
  return
fi
sleep 1
lsmod

# before drive ad9361, let's bring up duc and make sure dac is connected to ad9361 dma
SUBMODULE=tx_intf
if [ $last_input == "remote" ]
  then
    rm $SUBMODULE.ko
    sync
    wget ftp://192.168.10.1/driver/$SUBMODULE/$SUBMODULE.ko
    sync
fi
rmmod $SUBMODULE
insmod $SUBMODULE.ko

echo check $SUBMODULE module is loaded or not
checkModule $SUBMODULE
if [ $? -eq 1 ]
then
  return
fi
sleep 0.5



echo "set RF frontend"
# if [ $last_input == "remote" ]
#   then
#     rm rf_init.sh
#     sync
#     wget ftp://192.168.10.1/user_space/rf_init.sh
#     sync
#     chmod +x rf_init.sh
#     sync
# fi

# if [ $tx_offset_tuning_enable = "1" ]
# then
  ./rf_init_11n.sh
# else
#   ./rf_init.sh tx_offset_tuning_disable
# fi

#sleep 1

SUBMODULE=rx_intf
if [ $last_input == "remote" ]
  then
    rm $SUBMODULE.ko
    sync
    wget ftp://192.168.10.1/driver/$SUBMODULE/$SUBMODULE.ko
    sync
fi
rmmod $SUBMODULE
insmod $SUBMODULE.ko

echo check $SUBMODULE module is loaded or not
checkModule $SUBMODULE
if [ $? -eq 1 ]
then
  return
fi

SUBMODULE=openofdm_tx
if [ $last_input == "remote" ]
  then
    rm $SUBMODULE.ko
    sync
    wget ftp://192.168.10.1/driver/$SUBMODULE/$SUBMODULE.ko
    sync
fi
rmmod $SUBMODULE
insmod $SUBMODULE.ko

echo check $SUBMODULE module is loaded or not
checkModule $SUBMODULE
if [ $? -eq 1 ]
then
  return
fi

SUBMODULE=openofdm_rx
if [ $last_input == "remote" ]
  then
    rm $SUBMODULE.ko
    sync
    wget ftp://192.168.10.1/driver/$SUBMODULE/$SUBMODULE.ko
    sync
fi
rmmod $SUBMODULE
insmod $SUBMODULE.ko

echo check $SUBMODULE module is loaded or not
checkModule $SUBMODULE
if [ $? -eq 1 ]
then
  return
fi

SUBMODULE=xpu
if [ $last_input == "remote" ]
  then
    rm $SUBMODULE.ko
    sync
    wget ftp://192.168.10.1/driver/$SUBMODULE/$SUBMODULE.ko
    sync
fi
rmmod $SUBMODULE
insmod $SUBMODULE.ko

echo check $SUBMODULE module is loaded or not
checkModule $SUBMODULE
if [ $? -eq 1 ]
then
  return
fi
sleep 0.5

PROG=sdr
if [ $last_input == "remote" ]
  then
    rm $PROG.ko
    sync
    wget ftp://192.168.10.1/driver/$PROG.ko
    sync
fi

rmmod $PROG
echo insert $PROG.ko test_mode=$test_mode
insmod $PROG.ko test_mode=$test_mode

echo check $PROG module is loaded or not
checkModule $PROG
if [ $? -eq 1 ]
then
  return
fi

if ps -p $(</tmp/check_calib_inf.pid) > /dev/null
then
   kill $(</tmp/check_calib_inf.pid)
fi
./check_calib_inf.sh

echo the end
dmesg

# dmesg -c
# sleep 0.1
# ifconfig sdr0 up
