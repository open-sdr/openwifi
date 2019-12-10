#!/bin/bash

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
	last_input="xxxxxxxxx"
  test_mode=0
else
  if [ $1 -eq 1 ]
  then
    test_mode=1
  else
    test_mode=0
  fi
fi
echo last_input $last_input
echo test_mode $test_mode

modprobe mac80211

# dmesg -c

PROG=sdr
rmmod $PROG

SUBMODULE=xilinx_dma
if [ $last_input == "remote" ]
  then
    rm $SUBMODULE.ko
    sync
    wget ftp://192.168.10.1/driver/$SUBMODULE/$SUBMODULE.ko
    sync
fi
rmmod $SUBMODULE
insmod $SUBMODULE.ko

#sleep 1

echo check $SUBMODULE module is loaded or not
checkModule $SUBMODULE
if [ $? -eq 1 ]
then
  return
fi

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

SUBMODULE=ad9361_drv
if [ $last_input == "remote" ]
  then
    rm $SUBMODULE.ko
    sync
    wget ftp://192.168.10.1/driver/ad9361/$SUBMODULE.ko
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
sleep 1

echo "set RF frontend: ant0 rx, ant1 tx"
if [ $last_input == "remote" ]
  then
    rm rf_init.sh
    sync
    wget ftp://192.168.10.1/user_space/rf_init.sh
    sync
    chmod +x rf_init.sh
    sync
fi
./rf_init.sh

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

echo the end
dmesg

# dmesg -c
# sleep 0.1
# ifconfig sdr0 up



