#!/bin/bash

if [ $# -ne 3 ]
  then
    echo "Please input NIC_name ch_number ip_addr as input parameter!"
    exit
fi

nic_name=$1
ch_number=$2
ip_addr=$3
echo $nic_name
echo $ch_number
echo $ip_addr

sudo ip link set $nic_name down
sudo iwconfig $nic_name mode ad-hoc
sudo iwconfig $nic_name essid 'sdr-ad-hoc'
sudo ip link set $nic_name up
sudo iwconfig $nic_name channel $ch_number
#sudo iwconfig $nic_name modulation 11g
#sudo iwconfig $nic_name rate 6M
sudo ifconfig $nic_name $ip_addr netmask 255.255.255.0
ifconfig
iwconfig $nic_name
