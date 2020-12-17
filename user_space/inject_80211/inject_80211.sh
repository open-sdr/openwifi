#!/bin/bash

HW_MODE='n'
COUNT=100
DELAY=1000
RATE=( 0 1 2 3 4 5 6 7 )
SIZE=( $(seq -s' ' 50 100 1450) )
IF="mon0"

for (( i = 0 ; i < ${#SIZE[@]} ; i++ )) do
	for (( j = 0 ; j < ${#RATE[@]} ; j++ )) do
		inject_80211 -m $HW_MODE -n $COUNT -d $DELAY -r ${RATE[$j]} -s ${SIZE[$i]} $IF
		sleep 1
	done
done

