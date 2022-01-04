#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2021 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

if [ "$#" -lt 1 ]; then
    echo "You must enter 1 arguments: 1, 2, 3 or 4. For scan c1, c2, c2&c1 or c1&c2,"
    exit 1
fi

SCAN_OPTION=$1

if (($SCAN_OPTION == 1)); then
    echo "Scan tap1:"
    for j in {-64..63};
    do
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 0 $i 0 0
            sleep 0.01
        done
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 1 $i 0 0
            sleep 0.01
        done
    done
    exit 1
fi

if (($SCAN_OPTION == 2)); then
    echo "Scan tap2:"
    for j in {-64..63};
    do
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 0 0 0 $i
            sleep 0.01
        done
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 0 0 1 $i
            sleep 0.01
        done
    done
    exit 1
fi

if (($SCAN_OPTION == 3)); then
    echo "Scan tap1 after tap2:"
    for j in {-64..63};
    do
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 0 $j 0 $i
            # sleep 0.1
        done
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 0 $j 1 $i
            # sleep 0.1
        done
    done
    for j in {-64..63};
    do
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 1 $j 0 $i
            # sleep 0.1
        done
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 1 $j 1 $i
            # sleep 0.1
        done
    done
    exit 1
fi

if (($SCAN_OPTION == 4)); then
    echo "Scan tap2 after tap1:"
    for j in {-64..63};
    do
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 0 $i 0 $j
            # sleep 0.1
        done
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 1 $i 0 $j
            # sleep 0.1
        done
    done
    for j in {-64..63};
    do
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 0 $i 1 $j
            # sleep 0.1
        done
        for i in {-64..63};
        do
            ./csi_fuzzer.sh 1 $i 1 $j
            # sleep 0.1
        done
    done
    exit 1
fi
