#!/bin/bash

usage(){
    echo "Usage: $0 <interface connected to internet> <interface connected to openwifi board>"
}

sudo iptables -t nat -A POSTROUTING -o $1 -j MASQUERADE
sudo iptables -A FORWARD -i $2 -o $1 -j ACCEPT
sudo iptables -A FORWARD -i $1 -o $2 -m state --state RELATED,ESTABLISHED -j ACCEPT
