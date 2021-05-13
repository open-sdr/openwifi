  
#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2021 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -lt 4 ]; then
    echo "You must enter 4 arguments: c1_rot90_en c1_raw(-64 to 63) c2_rot90_en c2_raw(-64 to 63)"
    exit 1
fi

c1_rot90_en=$1
c1_raw=$2
c2_rot90_en=$3
c2_raw=$4

if (($c1_rot90_en != 0)) && (($c1_rot90_en != 1)); then
    echo "c1_rot90_en must be 0 or 1!"
    exit 1
fi

if (($c1_raw < -64)) || (($c1_raw > 63)); then
    echo "c1_raw must be -64 to 63!"
    exit 1
fi

if (($c2_rot90_en != 0)) && (($c2_rot90_en != 1)); then
    echo "c2_rot90_en must be 0 or 1!"
    exit 1
fi

if (($c2_raw < -64)) || (($c2_raw > 63)); then
    echo "c2_raw must be -64 to 63!"
    exit 1
fi

if (($c1_raw < 0)); then
    unsigned_c1=$(expr 128 + $c1_raw)
#    echo $unsigned_c1
else
    unsigned_c1=$c1_raw
fi

if (($c2_raw < 0)); then
    unsigned_c2=$(expr 128 + $c2_raw)
#    echo $unsigned_c2
else
    unsigned_c2=$c2_raw
fi

# echo $c1_rot90_en
# echo $unsigned_c1
# echo $c2_rot90_en
# echo $unsigned_c2

unsigned_dec_combined=$(($unsigned_c1 + 512 * $c1_rot90_en + 1024 * $unsigned_c2 + 524288 * $c2_rot90_en))
# echo $unsigned_dec_combined

echo "./sdrctl dev sdr0 set reg tx_intf 5 $unsigned_dec_combined"
./sdrctl dev sdr0 set reg tx_intf 5 $unsigned_dec_combined
