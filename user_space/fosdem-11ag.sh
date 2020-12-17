#!/bin/bash

killall hostapd
killall webfsd

cd ~/openwifi
service network-manager stop
./wgd.sh
ifconfig sdr0 192.168.13.1
route add default gw 192.168.10.1
service isc-dhcp-server restart
hostapd hostapd-openwifi-11ag.conf &
sleep 5
cd webserver
webfsd -F -p 80 -f index.html &
