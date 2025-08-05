<!--
Author: Xianjun jiao
SPDX-FileCopyrightText: 2019 UGent
SPDX-License-Identifier: AGPL-3.0-or-later
-->

Counter/statistics (number of TX packet, RX packet, watchdog event, etc.) in FPGA is offered via register write/read.

[[PHY RX TX event counter in side channel](#PHY-RX-TX-event-counter-in-side-channel)]
[[PHY RX watchdog event counter in openofdm rx](#PHY-RX-watchdog-event-counter-in-openofdm-rx)]

## PHY RX watchdog event counter in openofdm rx
There is a signal_watchdog module inside openofdm_rx to detect the abnormal signal as early as possible, so that the receiver will not be busy with fake/abnormal signal for long time. (If the receiver is attracted by fake/abnormal signal easily, it could miss the normal/target packet).

To access the watchdog event counter in openofdm_rx, [sdrctl command](../README.md#sdrctl-command) is used.

To select the event you are interested in:
```
sdrctl dev sdr0 set reg rx 17 event_type
```
The event_type options:
- 0: phase_offset(sync_short) too big
- 1: Too many equalizer out small values
- 2: DC/slow-sine-wave is detected
- 3: Packet too short
- 4: Packet too long

To read the event counter (selected by register 17 above):
```
sdrctl dev sdr0 get reg rx 30
```
Write any value to above register 30 will clear the selected event counter (by register 17).

## PHY RX TX event counter in side channel
The 1st step is alway loading the side channel kernel module:
```
insmod side_ch.ko
```

The register write command is:
```
./side_ch_ctl whXdY
X -- register index
Y -- decimal value to be written
./side_ch_ctl whXhY
X -- register index
Y -- hex value to be written (useful for MAC address)
```
Write register 26~31 with arbitrary value to reset the corresponding counter to 0.

The register read command is:
```
./side_ch_ctl rhX
X -- register index
```

**Register definition:**

The register 26~31 readback value represents the number of event happened. Each register has two event sources that can be selected via bit in register 19.

register idx|source selection reg19|event
------------|----------------------|-----------
26          |reg19[0] == 0         |short_preamble_detected
26          |reg19[0] == 1         |phy_tx_start
27          |reg19[4] == 0         |long_preamble_detected
27          |reg19[4] == 1         |phy_tx_done
28          |reg19[8] == 0         |pkt_header_valid_strobe
28          |reg19[8] == 1         |rssi_above_th
29          |reg19[12] == 0        |pkt_header_valid_strobe&pkt_header_valid
29          |reg19[12] == 1        |gain_change
30          |reg19[16] == 0        |((fcs_in_strobe&addr2_match)&pkt_for_me)&is_data
30          |reg19[16] == 1        |agc_lock
31          |reg19[20] == 0        |(((fcs_in_strobe&fcs_ok)&addr2_match)&pkt_for_me)&is_data
31          |reg19[20] == 1        |tx_pkt_need_ack

Note: fcs_in_strobe means decoding is done (not necessarily CRC is correct); fcs_ok 1 means CRC correct; fcs_ok 0 means CRC not correct.

Note: addr2_match means addr2 matches to the register (addr2_target) value; pkt_for_me means addr1 matches self mac addr; is_data means the packet type is data.

Configuration register:

register idx|meaning               |note
------------|----------------------|-----------
7           |addr2 target value    |fcs event always needs addr2 match
9           |threshold for event rssi_above_th|check auto_lbt_th in ad9361_rf_set_channel of sdr.c to estimate a proper value

Note: addr2 (source/sender's MAC address) target setting uses only 32bit. For address 6c:fd:b9:4c:b1:c1, you set b94cb1c1

Note: read register 62 of xpu for some addr2 captured by the receiver
