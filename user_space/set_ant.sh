#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo "Need 2 arguments. The 1st select rx antenna. The 2nd for tx antenna."
    echo "Eg, "
    echo "./set_ant rx2 tx2"
    echo "./set_ant rx1 tx1"
    echo "./set_ant rx2 tx1"
    echo "./set_ant rx1 tx2"
    exit 1
fi

set -x
if [ $1 = "rx2" ] 
then
    ./sdrctl dev sdr0 set reg drv_rx 1 1
elif [ $1 = "rx1" ] 
then
    ./sdrctl dev sdr0 set reg drv_rx 1 0
else
    echo "The 1st argument must be rx2 or rx1!"
    exit 1
fi

if [ $2 = "tx2" ] 
then
    ./sdrctl dev sdr0 set reg drv_tx 1 1
elif [ $2 = "tx1" ] 
then
    ./sdrctl dev sdr0 set reg drv_tx 1 0
else
    echo "The 2nd argument must be tx2 or tx1!"
    exit 1
fi

ifconfig sdr0 down
ifconfig sdr0 up
