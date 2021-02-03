#!/bin/bash

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

./sdrctl dev sdr0 get reg xpu 19

./sdrctl dev sdr0 set reg xpu 19 3

