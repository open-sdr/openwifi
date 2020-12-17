
## 802.11 packet injection

The Linux wireless networking stack (i.e. driver, mac80211, cfg80211, net_dev, user app) is a robust implementation supporting a plethora of wireless devices. As robust as it is, it also has a drawback when it comes to single-layer testing. 

Ping and Iperf are well established performance measurement tools. However, using such tools to measure 802.11 PHY performance can be misleading, simply because they touch multiple layers in the network stack. 

Luckily, the mac80211 Linux subsystem provides packet injection functionality and it allows us to have finer control over physical layer testing.

To this end, we have adapted a [packetspammer](https://github.com/gnychis/packetspammer) application originally written by Andy Green <andy@warmcat.com> and maintained by George Nychis <gnychis@gmail.com>.

### inject_80211
Userspace program to inject 802.11 packets through mac80211 supported (softmac) wireless devices.

### Options
  ```
-m/--hw_mode <hardware operation mode> (a,g,n)
-r/--rate_index <rate/MCS index> (0,1,2,3,4,5,6,7)
-i/--sgi_flag (0,1)
-n/--num_packets <number of packets>
-s/--payload_size <payload size in bytes>
-d/--delay <delay between packets in usec>
-h   this menu
  ```

### Example:
```
iw dev wlan0 interface add mon0 type monitor && ifconfig mon0 up
inject_80211 -m n -r 0  -n 64 -s 100 mon0     # Inject 10 802.11n packets at 6.5Mbps bitrate and 64bytes size
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
iw dev wlan0 interface add mon0 type monitor && ifconfig mon0 up
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

