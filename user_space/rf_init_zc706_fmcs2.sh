#!/bin/sh

home_dir=$(pwd)

cd /sys/bus/iio/devices/iio:device2

echo "rx bw"
cat in_voltage_rf_bandwidth
#echo 37500000 >  in_voltage_rf_bandwidth
echo 17500000 >  in_voltage_rf_bandwidth
cat in_voltage_rf_bandwidth
sync

echo "tx_bw"
cat out_voltage_rf_bandwidth
echo 37500000 >  out_voltage_rf_bandwidth
#echo 40000000 >  out_voltage_rf_bandwidth
#echo 20000000 >  out_voltage_rf_bandwidth
cat out_voltage_rf_bandwidth
sync
#sleep 0.5

echo "rx rate"
cat in_voltage_sampling_frequency
echo 40000000 >  in_voltage_sampling_frequency
#echo 20000000 >  in_voltage_sampling_frequency
cat in_voltage_sampling_frequency
sync

echo "tx rate"
cat out_voltage_sampling_frequency
echo 40000000 >  out_voltage_sampling_frequency
#echo 20000000 >  out_voltage_sampling_frequency
cat out_voltage_sampling_frequency
sync
sleep 1

echo "rx lo"
cat out_altvoltage0_RX_LO_frequency
#echo 2427000000 >  out_altvoltage0_RX_LO_frequency
echo 5240000000 >  out_altvoltage0_RX_LO_frequency
#echo 2320000000 >  out_altvoltage0_RX_LO_frequency
cat out_altvoltage0_RX_LO_frequency
sync

echo "tx lo"
cat out_altvoltage1_TX_LO_frequency
#echo 2447000000 >  out_altvoltage1_TX_LO_frequency
echo 5250000000 >  out_altvoltage1_TX_LO_frequency
#echo 2320000000 >  out_altvoltage1_TX_LO_frequency
cat out_altvoltage1_TX_LO_frequency
sync
#sleep 1

echo "rx0 agc fast_attack"
#echo "rx0 agc manual"
cat in_voltage0_gain_control_mode
echo fast_attack > in_voltage0_gain_control_mode
#echo manual > in_voltage0_gain_control_mode
cat in_voltage0_gain_control_mode
sync

echo "rx1 agc fast_attack"
#echo "rx1 agc manual"
cat in_voltage1_gain_control_mode
echo fast_attack > in_voltage1_gain_control_mode
#echo manual > in_voltage1_gain_control_mode
cat in_voltage1_gain_control_mode
sync
sleep 1

echo "rx0 gain to 70" # this set gain is gpio gain - 5dB (test with agc and read back gpio in driver)
cat in_voltage0_hardwaregain
echo 70 > in_voltage0_hardwaregain
cat in_voltage0_hardwaregain
sync

echo "rx1 gain to 70"
cat in_voltage1_hardwaregain
echo 70 > in_voltage1_hardwaregain
cat in_voltage1_hardwaregain
sync

echo "tx0 gain -89dB"
cat out_voltage0_hardwaregain
echo -89 > out_voltage0_hardwaregain
cat out_voltage0_hardwaregain
sync

echo "tx1 gain 0dB"
cat out_voltage1_hardwaregain
echo 0 > out_voltage1_hardwaregain
cat out_voltage1_hardwaregain
sync

echo "rssi"
cat in_voltage0_rssi
cat in_voltage1_rssi

cd $home_dir
