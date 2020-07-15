killall hostapd
killall webfsd

service network-manager stop
./wgd.sh 
iwconfig sdr0 mode managed
ifconfig sdr0 up
iwconfig sdr0 essid openwifi
./sdrctl dev sdr0 set reg drv_rx 7 2
