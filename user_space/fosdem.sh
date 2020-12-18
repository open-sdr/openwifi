#!/bin/bash

killall hostapd
killall webfsd

cd ~/openwifi
service network-manager stop
./wgd.sh
ifconfig sdr0 192.168.13.1
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
service isc-dhcp-server restart
hostapd hostapd-openwifi.conf &
sleep 5
cd webserver
webfsd -F -p 80 -f index.html &
