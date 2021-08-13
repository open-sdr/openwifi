#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -ne 1 ]; then
    echo "You must enter the \$OPENWIFI_DIR (the openwifi root directory) as argument"
    exit 1
fi
OPENWIFI_DIR=$1


if [ -f "$OPENWIFI_DIR/LICENSE" ]; then
    echo "\$OPENWIFI_DIR is found!"
else
    echo "\$OPENWIFI_DIR is not correct. Please check!"
    exit 1
fi

home_dir=$(pwd)

set -ex

cd $OPENWIFI_DIR/
git submodule init openwifi-hw
git submodule update openwifi-hw
cd openwifi-hw
git checkout antsdr
git pull

cd $home_dir
