<!--
Author: Michael Mehari, Xianjun Jiao
SPDX-FileCopyrightText: 2019 UGent
SPDX-License-Identifier: AGPL-3.0-or-later
-->

## 802.11 packet injection and fuzzing

The Linux wireless networking stack (i.e. driver, mac80211, cfg80211, net_dev, user app) is a robust implementation supporting a plethora of wireless devices. As robust as it is, it also has a drawback when it comes to single-layer testing and manual/total control mode (fuzzing). 

Ping and Iperf are well established performance measurement tools. However, using such tools to measure 802.11 PHY performance can be misleading, simply because they touch multiple layers in the network stack. 

Luckily, the mac80211 Linux subsystem provides packet injection functionality when the NIC is in the monitor mode and it allows us to have finer control for physical layer testing and/or fuzzing.

Besides the traditional fuzzing tool (like scapy), we have adapted a [packetspammer](https://github.com/gnychis/packetspammer) application, which is originally written by Andy Green <andy@warmcat.com> and maintained by George Nychis <gnychis@gmail.com>, to show how to inject packets and control the FPGA behavior.

### Build inject_80211 on board
Userspace program to inject 802.11 packets through mac80211 supported (softmac) wireless devices.

Login/ssh to the board and setup internet connection according to the Quick Start. Then
```
cd openwifi/inject_80211
make
```
### Customize the packet content
To customize the packet, following piece of the inject_80211.c needs to be changed:
```
/* IEEE80211 header */
static u8 ieee_hdr_data[] =
{
	0x08, 0x02, 0x00, 0x00,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x22,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x33,
	0x10, 0x86,
};

static u8 ieee_hdr_mgmt[] =
{
	0x00, 0x00, 0x00, 0x00,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x22,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x33,
	0x10, 0x86,
};

static u8 ieee_hdr_ack_cts[] =
{
	0xd4, 0x00, 0x00, 0x00,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
};

static u8 ieee_hdr_rts[] =
{
	0xb4, 0x00, 0x00, 0x00,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x22,
};
```
Note: The byte/bit order might not be intuitive when comparing with the standard.

### FPGA behavior control
- ACK and retransmission after FPGA sends packet

In openwifi_tx of sdr.c, many FPGA behaviors can be controled. Generally they are controled by the information from upper layer (Linux mac80211), but you can override them in driver (sdr.c)

If 802.11 ACK is expected from the peer after the packet is sent by FPGA, variable **pkt_need_ack** should be overridden to 1. In this case, the FPGA will try to receive ACK, and report the sending status (ACK is received or not) to upper layer (Linux mac80211)

The maximum times of transmission for the packet can be controled by variable **retry_limit_raw**. If no ACK is received after the packet is sent, FPGA will try retransmissions automatically if retry_limit_raw>1.

- ACK after FPGA receives packet in monitor mode

Even in monitor mode, openwifi FPGA still sends ACK after the packet is received, if the conditions are met: MAC address is matched, it is a data frame, etc. To disable this automatic ACK generation, the register 11 of xpu should be set to 16:
```
sdrctl dev sdr0 set reg xpu 11 16 
```

### Options of program inject_80211
```
-m/--hw_mode <hardware operation mode> (a,g,n)
-r/--rate_index <rate/MCS index> (0,1,2,3,4,5,6,7)
-t/--packet_type (m/c/d/r for management/control/data/reserved)
-e/--sub_type (hex value. example:
     8/A/B/C for Beacon/Disassociation/Authentication/Deauth, when packet_type m
     A/B/C/D for PS-Poll/RTS/CTS/ACK, when packet_type c
     0/1/2/8 for Data/Data+CF-Ack/Data+CF-Poll/QoS-Data, when packet_type d)
-a/--addr1 <the last byte of addr1 in hex>
-b/--addr2 <the last byte of addr2 in hex>
-i/--sgi_flag (0,1)
-n/--num_packets <number of packets>
-s/--payload_size <payload size in bytes>
-d/--delay <delay between packets in usec>
-h   this menu
```

### Example:
Login/ssh to the board, Then
```
cd openwifi
./wgd.sh
./monitor_ch.sh sdr0 11
(Above will turn sdr0 into the monitor mode and monitor on channel 11)
./inject_80211/inject_80211 -m n -r 0 -n 10 -s 64 sdr0
(Above will inject 10 802.11n packets at 6.5Mbps bitrate and 64bytes size via NIC sdr0)
```
When above injection command is running, you could see the injected packets with wireshark (or other packet sniffer) on another WiFi device monitoring channel 11.

Or add extra virtual monitor interface on top of sdr0, and inject packets:
```
iw dev sdr0 interface add mon0 type monitor && ifconfig mon0 up
./inject_80211/inject_80211 -m n -r 0 -n 10 -s 64 mon0     # Inject 10 802.11n packets at 6.5Mbps bitrate and 64bytes size
```

### Link performance test

To make a profound experimental analysis on the physical layer performance, we can rely on automation scripts.

The following script will inject 100 802.11n packets at different bitrates and payload sizes.

```
#!/bin/bash

HW_MODE='n'
COUNT=100
DELAY=1000
RATE=( 0 1 2 3 4 5 6 7 )
SIZE=( $(seq -s' ' 50 100 1450) ) # paload size in bytes
IF="mon0"

for (( i = 0 ; i < ${#PAYLOAD[@]} ; i++ )) do
	for (( j = 0 ; j < ${#RATE[@]} ; j++ )) do
		inject_80211 -m $HW_MODE -n $COUNT -d $DELAY -r ${RATE[$j]} -s ${SIZE[$i]} $IF
		sleep 1
	done
done

```

On the receiver side, we can use tcpdump to collect the pcap traces.

```
iw dev sdr0 interface add mon0 type monitor && ifconfig mon0 up
tcpdump -i mon0 -w trace.pcap 'wlan addr1 ff:ff:ff:ff:ff:ff and wlan addr2 66:55:44:33:22:11'
```

Wlan addresses *ff:ff:ff:ff:ff:ff* and *66:55:44:33:22:11* are specific to our injector application.

Next, we analyze the collected pcap traces using the analysis tool provided.

```
analyze_80211 trace.pcap
```

An excerpt from a sample analysis looks the following

```
HW MODE	RATE(Mbps)	SGI	SIZE(bytes)	COUNT	Duration(sec)
=======	==========	===	===========	=====	=============
802.11n	6.5           	OFF	54		100	0.11159
802.11n	13.0		OFF	54		100	0.11264
802.11n	19.5		OFF	54		100	0.11156
802.11n	26.0		OFF	54	    	100	0.11268
802.11n	39.0		OFF	54	    	100	0.11333
802.11n	52.0		OFF	54	    	100	0.11149
802.11n	58.5		OFF	54	    	100	0.11469
802.11n	65.0		OFF	54	    	100	0.11408
```

