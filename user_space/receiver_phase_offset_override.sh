#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2023 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

# To avoid override, just run without any arguments
# To       override, example: ./receiver_phase_offset_override.sh  -8

set -x

if [[ -n $1 ]]; then
  phase_offset=$1
else
  echo "Disable phase offset override by setting bit31 to 0"
  reg_val=$((0<<31))
  printf "0x%X\n" $reg_val
  echo "./sdrctl dev sdr0 set reg rx 19 $reg_val"
  ./sdrctl dev sdr0 set reg rx 19 $reg_val
  exit 0
fi

echo $phase_offset
printf "0x%X\n" $phase_offset

reg_val=$(( ($phase_offset & 65535) | (1 << 31) ))
printf "0x%X\n" $reg_val

./sdrctl dev sdr0 set reg rx 19 $reg_val

set +x
