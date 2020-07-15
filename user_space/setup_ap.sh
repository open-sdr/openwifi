killall hostapd
killall webfsd

./fosdem.sh
./sdrctl dev sdr0 set reg drv_xpu 0 1
./sdrctl dev sdr0 set reg drv_rx 7 2
