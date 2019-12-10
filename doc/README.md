# openwifi domument
<img src="./openwifi-detail.jpg" width="1100">

Above figure shows software and hardware/FPGA modules that compose the openwifi design. The module name is equal/similar to the source code file name. Driver modules source code are in openwifi/driver/. FPGA modules source code are in openwifi-hw repository. The user space tool sdrctl source code are in openwifi/user_space/sdrctl_src/.

**sdrctl command**

Besides the Linux native Wi-Fi control programs, such as ifconfig/iw/iwconfig/iwlist/wpa_supplicant/hostapd/etc, openwifi offers a user space tool sdrctl to access openwifi specific functionalities. sdrctl is implemented as nl80211 testmode command and communicates with openwifi driver (function openwifi_testmode_cmd in sdr.c) via Linux nl80211--cfg80211--mac80211 path 

* **get and set a parameter**
```
sdrctl dev sdr0 get para_name
sdrctl dev sdr0 set para_name value 
```
para_name|meaning|example
---------|-------|----
addr0|target MAC addres of tx slice 0|32bit. for address 6c:fd:b9:4c:b1:c1, you set b94cb1c1
slice_total0|tx slice 0 cycle length in us|for length 50ms, you set 49999
slice_start0|tx slice 0 cycle start time in us|for start at 10ms, you set 10000
slice_end0|  tx slice 0 cycle end   time in us|for end   at 40ms, you set 39999
addr1|target MAC addres of tx slice 1|32bit. for address 6c:fd:b9:4c:b1:c1, you set b94cb1c1
slice_total1|tx slice 1 cycle length in us|for length 50ms, you set 49999
slice_start1|tx slice 1 cycle start time in us|for start at 10ms, you set 10000
slice_end1|  tx slice 1 cycle end   time in us|for end   at 40ms, you set 39999

* **get and set a register of a module**
```
sdrctl dev sdr0 get reg module_name reg_idx
sdrctl dev sdr0 set reg module_name reg_idx reg_value 
```
module_name drv_rx/drv_tx/drv_xpu refer to driver modules. Related registers are defined in sdr.h (drv_rx_reg_val/drv_tx_reg_val/drv_xpu_reg_val)

module_name rf/rx_intf/tx_intf/rx/tx/xpu refer to RF (ad9xxx front-end) and FPGA (rx_intf/tx_intf/openofdm_rx/openofdm_tx/xpu) modules. Related register addresses are defined in hw_def.h.

module_name: drv_rx

reg_idx|meaning|example
-------|-------|----
1|rx antenna selection|0:rx1, 1:rx2. After this command, you should down and up sdr0 by ifconfig, but not reload sdr0 driver via ./wgd.sh

module_name: drv_tx

reg_idx|meaning|example
-------|-------|----
0|override Linux rate control of tx unicast data packet|4:6M, 5:9M, 6:12M, 7:18M, 8:24M, 9:36M, 10:48M, 11:54M
1|tx antenna selection|0:tx1, 1:tx2. After this command, you should down and up sdr0 by ifconfig, but not reload sdr0 driver via ./wgd.sh

module_name: drv_xpu

reg_idx|meaning|example
-------|-------|----
x|x|x

module_name: rf

reg_idx|meaning|example
-------|-------|----
x|x|x

module_name: rx_intf

reg_idx|meaning|example
-------|-------|----
2|enable/disable rx interrupt|256(0x100):disable, 0:enable

module_name: tx_intf

reg_idx|meaning|example
-------|-------|----
13|tx I/Q digital gain before DUC|current optimal value: 237
14|enable/disable tx interrupt|196672(0x30040):disable, 64(0x40):enable

module_name: rx

reg_idx|meaning|example
-------|-------|----
20|history of PHY rx state|read only. If the last digit readback is always 3, it means the Viterbi decoder stops working

module_name: tx

reg_idx|meaning|example
-------|-------|----
1|pilot scrambler initial state|lowest 7 bits are used. 0x7E by default in openofdm_tx.c
2|data  scrambler initial state|lowest 7 bits are used. 0x7F by default in openofdm_tx.c

module_name: xpu

reg_idx|meaning|example
-------|-------|----
2|TSF timer low  32bit write|only write this register won't trigger the TSF timer reload. should use together with register for high 32bit
3|TSF timer high 32bit write|falling edge of MSB will trigger the TSF timer reload, which means write '1' then '0' to MSB
4|band and channel number setting|see enum openwifi_band in hw_def.h. it will be set automatically by Linux. normally you shouldn't set it
11|max number of retransmission in FPGA|normally number of retransmission controled by Linux in real-time. If you write non-zeros value to this register, it will override Linux real-time setting
19|CSMA enable/disable|3758096384(0xe0000000): disable, 3:enable
20|tx slice 0 cycle length in us|for length 50ms, you set 49999
21|tx slice 0 cycle start time in us|for start at 10ms, you set 10000
22|tx slice 0 cycle end   time in us|for end   at 40ms, you set 39999
23|tx slice 1 cycle length in us|for length 50ms, you set 49999
24|tx slice 1 cycle start time in us|for start at 10ms, you set 10000
25|tx slice 1 cycle end   time in us|for end   at 40ms, you set 39999
27|FPGA packet filter config|check openwifi_configure_filter in sdr.c. also: https://www.kernel.org/doc/html/v4.9/80211/mac80211.html#frame-filtering
28|BSSID address low  32bit for BSSID filtering|normally it is set by Linux in real-time automatically
29|BSSID address high 32bit for BSSID filtering|normally it is set by Linux in real-time automatically
30|openwifi MAC address low  32bit|
31|openwifi MAC address high 32bit|check XPU_REG_MAC_ADDR_write in sdr.c to see how we set MAC address to FPGA when NIC start
58|TSF runtime value low  32bit|read only
59|TSF runtime value high 32bit|read only
