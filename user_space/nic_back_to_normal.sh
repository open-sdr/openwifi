#!/bin/bash

if [ $# -ne 1 ]
  then
    echo "Please input NIC name as input parameter!"
    exit
fi

nic_name=$1
echo $nic_name

# sudo service network-manager stop
sudo ip link set $nic_name down
sudo iwconfig $nic_name mode managed
#sudo iwconfig $nic_name modulation 11g
sudo ip link set $nic_name up
ifconfig
iwconfig $nic_name

