#!/bin/bash

PL_MIN=100
PL_INC=100
PL_MAX=1500
PAYLOAD=( $(seq -s' ' $PL_MIN $PL_INC $PL_MAX) ) # paload size in bytes
MCS_BPS=( 6 9 12 18 24 36 48 54 )
MCS_IDX=( 4 5  6  7  8  9 10 11 )

INTERVAL=0.001	# Wait interval seconds between sending each packet
PKT_CNT=700		# Stop after sending count ECHO_REQUEST packets
DEADLINE=1		# Specify a timeout, in seconds, before ping exits regardless of how many packets have been sent or received

SDRCTL_EXEC="./sdrctl_src/sdrctl"
CLIENT_IP="192.168.13.2"

# Bandwidth = 1.4 MHz
printf "LINK PERFORMANCE TEST\n"
printf "=====================\n"
printf "RATE/PL\t"
for (( j = 0 ; j < ${#PAYLOAD[@]} ; j++ )) do
	printf "%*s" 12 "${PAYLOAD[j]}"
done
printf "\n"

for (( i = 0 ; i < ${#MCS_IDX[@]} ; i++ )) do

	# configure MCS
	$SDRCTL_EXEC dev sdr0 set reg drv_tx 0 ${MCS_IDX[$i]} > /dev/null

	printf "%sMbps\t" ${MCS_BPS[$i]}
	for (( j = 0 ; j < ${#PAYLOAD[@]} ; j++ )) do

		# Measure link performance
		link_per_str=$(ping $CLIENT_IP -i $INTERVAL -c $PKT_CNT -w $DEADLINE -s ${PAYLOAD[$j]} -nq | while read line; do

			# Skip non packet-loss and non rtt responses
			[[ ! "$line" =~ "packet loss" ]] && [[ ! "$line" =~ "rtt" ]] && continue

			# Extract packet loss
			if [[ "$line" =~ "packet loss" ]]; then
				PL=$(echo $line | grep 'packet loss' | cut -d' ' -f6)
				if [[ $PL == "100%" ]]; then
					printf "%s,INF" $PL
				else
					printf "%s," $PL
				fi
			# Extract rtt
			else
				RTT=$(echo $line | grep rtt | cut -d/ -f5)
				printf "%s" $RTT
			fi

		done)
		printf "%*s" 12 $link_per_str
	done
	printf "\n"
done

