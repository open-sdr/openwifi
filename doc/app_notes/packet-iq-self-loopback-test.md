<!--
Author: Xianjun Jiao
SPDX-FileCopyrightText: 2022 UGent
SPDX-License-Identifier: AGPL-3.0-or-later
-->

One super power of the openwifi platform is "**Full Duplex**" which means that openwifi baseband can receive its own TX signal.
This makes the IQ sample and WiFi packet self loopback test possible. Reading the normal IQ sample capture [app note](iq.md) will help if you have issue or 
want to understand openwifi side channel (for IQ and CSI) deeper.
![](./openwifi-loopback-principle.jpg)

[[IQ self loopback quick start](#IQ-self-loopback-quick-start)]
[[Check the packet loopback on board](#Check-the-packet-loopback-on-board)]
[[Self loopback config](#Self-loopback-config)]

## IQ self loopback quick start
(Please replace the IQ length **8187** by **4095** if you use low end FPGA board: zedboard/adrv9464z7020/antsdr/zc702)
- Power on the SDR board.
- Put the Tx and Rx antenna as close as possible.
- Connect a computer to the SDR board via Ethernet cable. The computer should have static IP 192.168.10.1. Open a terminal on the computer, and then in the terminal:
  ```
  ssh root@192.168.10.122
  (password: openwifi)
  cd openwifi
  ./wgd.sh
  (Bring up the openwifi NIC sdr0)
  ./monitor_ch.sh sdr0 44
  (Setup monitor mode in WiFi channel 44. You should find a channel as clean as possible in your location)
  insmod side_ch.ko iq_len_init=8187
  ./side_ch_ctl wh11d0
  (Set 0 to register 11. It means the pre trigger length is 0, so we only capture IQ after trigger condition is met)
  ./side_ch_ctl wh8d16
  (Set 16 to register 8 -- set trigger condition to phy_tx_started signal from openofdm tx core)
  ./sdrctl dev sdr0 set reg xpu 1 1
  (Unmute the baseband self-receiving to receive openwifi own TX signal/packet -- important for self loopback!)
  ./side_ch_ctl wh5h0
  (Set the loopback mode to over-the-air)
  ./side_ch_ctl g0
  (Relay the FPGA IQ capture to the host computer that will show the captured IQ later on)
  ```
  You should see on outputs like:
  ```
  loop 22848 side info count 0
  loop 22912 side info count 0
  ...
  ```
  Now the count is always 0, because we haven't instructed openwifi to send packet for loopback test.
  
- Leave above ssh session untouched. Open a new ssh session to the board from your computer. Then run on board:
  ```
  cd openwifi/inject_80211/
  make
  (Build our example packet injection program)
  ./inject_80211 -m n -r 5 -n 1 sdr0
  (Inject one packet to openwifi sdr0 NIC)
  ```
  Normally in the previous ssh session, the count becomes 1. It means one packet (of IQ sample) is sent and captured via loopback over the air.

- On your computer (NOT ssh onboard!), run:
  ```
  cd openwifi/user_space/side_ch_ctl_src
  python3 iq_capture.py 8187
  ```
  You might need to install beforehand: "sudo apt install python3-numpy", and "sudo apt install python3-matplotlib".
  
- Leave the above host session untouched. Let's go to the second ssh session (packet injection), and do single packet Tx again:
  ```
  ./inject_80211 -m n -r 5 -n 1 sdr0
  ```
  Normally in the 1st ssh session, the count becomes 2. You should also see IQ sample capture figures like this:
  ![](./openwifi-iq-loopback.jpg)
  
- Stop the python3 script, which plots above, in the host session. A file **iq.txt** is generated. You can use the Matlab script test_iq_file_display.m 
to do further offline analysis, or feed the IQ sample to the openwifi receiver simulation, etc.

## Check the packet loopback on board

- While signal/packet is looped back, you can capture it on board via normal sniffer program for further check/analysis on the packet (bit/byte level instead of IQ level), such as tcpdump or tshark.
  A new ssh session to the board should be opened to do this before running the packet injection:
  ```
  tcpdump -i sdr0
  ```
  Run the packet injection "./inject_80211 -m n -r 5 -n 1 sdr0" in another session, you should see the packet information printed by tcpdump from self over-the-air loopback.
  
- You can also see the openwifi printk message of Rx packet (self Tx looped back) while the packet comes to the openwifi Rx interrupt.
  A new ssh session to the board should be opened to do this before running the packet injection:
  ```
  cd openwifi
  ./sdrctl dev sdr0 set reg drv_rx 7 7
  ./sdrctl dev sdr0 set reg drv_tx 7 7
  (Turn on the openwifi Tx/Rx printk logging)
  ```
  Stop the "./side_ch_ctl g0" in the very first ssh session. Run the packet injection, then check the printk message:
  ```
  ./inject_80211/inject_80211 -m n -r 5 -n 1 sdr0
  dmesg
  ```
  You should see the printk message of packet Tx and Rx from the openwifi driver (sdr.c).

## Self loopback config

- By default, the loopback is via the air (from Tx antenna to Rx antenna). FPGA inernal loopback option is offered to have IQ sample and packet without 
  any interference. To have FPGA internal loopback, replace the "./side_ch_ctl wh5h0" during setup (the very 1st ssh session) by:
  ```
  ./side_ch_ctl wh5h4
  ```
- Lots of packet injection parameters can be set: number of packet, type (data/control/management), MCS/rate, size, interval, etc. Please run the packet injection
  program without any arguments to see the help.
  
- Besides the packet Tx via injection over monitor mode for loopback test, normal WiFi mode (AP/Client/ad-hoc) can also run together with self loopback. 
  For instance, run **fosdem.sh** instead of **wgd.sh** to setup an openwifi AP that will transmit beacons. The wgd.sh can also be replaced with other scenario
  setup scripts. Please check [Application notes](README.md)

- To understand deeper of all above commands/settings, please refer to [Capture IQ sample, AGC gain, RSSI with many types of trigger condition](iq.md) and
  [Capture dual antenna TX/RX IQ for multi-purpose (capture collision)](iq_2ant.md)
