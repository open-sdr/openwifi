#!/bin/bash

rssi_raw=$(./sdrctl dev sdr0 get reg xpu 57)
echo $rssi_raw

rssi_raw=${rssi_raw: -8}
echo $rssi_raw

rssi_raw_dec=$(( 16#$rssi_raw ))
echo $rssi_raw_dec

#rssi_half_db=$(expr (16#$rss_raw) \& 2047)
#rssi_half_db=$(($rssi_raw_dec & 2047))
#rssi_half_db=$(($rssi_raw_dec & 16#7ff))
#the low 11 bits are rssi_half_db
rssi_half_db=$((16#$rssi_raw & 16#7ff))
echo $rssi_half_db

