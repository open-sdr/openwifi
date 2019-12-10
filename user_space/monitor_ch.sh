#!/bin/bash

if [ $# -ne 2 ]
  then
    echo "Please input NIC_name ch_number as input parameter!"
    exit
fi

nic_name=$1
ch_number=$2
echo $nic_name
echo $ch_number

# sudo service network-manager stop
sudo ip link set $nic_name down
sudo iwconfig $nic_name mode monitor
sudo ip link set $nic_name up
sudo iwconfig $nic_name channel $ch_number
# sudo iwconfig $nic_name modulation 11g
# sudo iwconfig $nic_name rate 6M
ifconfig
iwconfig $nic_name

