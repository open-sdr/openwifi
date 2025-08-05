Some usual/frequent control trick over the openwifi FPGA. You need to do these controls on board in the openwifi directory.

[[CCA LBT threshold and disable](#CCA-LBT-threshold-and-disable)]
[[Retransmission and ACK control](#Retransmission-and-ACK-control)]
[[NAV DIFS EIFS CW disable and enable](#NAV-DIFS-EIFS-CW-disable-and-enable)]
[[CW max and min config](#CW-max-and-min-config)]

[[Rx gain config](#Rx-gain-config)]
[[Tx power config](#Tx-power-config)]
[[Tx Lo and port config](#Tx-Lo-and-port-config)]
[[Antenna selection](#Antenna-selection)]
[[Restrict the frequency](#Restrict-the-frequency)]
[[Receiver sensitivity control](#Receiver-sensitivity-control)]

[[Tx rate config](#Tx-rate-config)]
[[Arbitrary Tx IQ sample](#Arbitrary-Tx-IQ-sample)]

## CCA LBT threshold and disable

In normal operation, different threshold is set to FPGA according to the different calibration of different frequency/channel by driver automatically. Show the current LBT threshold in FPGA:
```
./set_lbt_th.sh
dmesg
```
It shows: "sdr,sdr FPGA LBT threshold 166(-62dBm). The last_auto_fpga_lbt_th 166(-62dBm). rssi corr 145". Check rssi_half_db_to_rssi_dbm()/rssi_dbm_to_rssi_half_db() in sdr.c to see the relation to rssi dBm.

Override a new threshold -NNdBm to FPGA, for example -70dBm:
```
./set_lbt_th.sh 70
dmesg
```
Above will disable the automatic CCA threshold setting from the openwifi driver.

Recover the driver automatic control on the threshold:
```
./set_lbt_th.sh 0
dmesg
```
Disable the CCA by setting a very strong level as threshold, for example -1dBm:
```
./set_lbt_th.sh 1
dmesg
```
After above command, the CCA engine will always believe the channel is idle, because the rx signal strength not likely could exceed -1dBm.
  
## Retransmission and ACK control

The best way of override the maximum number of re-transmission for a Tx packet is doing it in the driver openwifi_tx() function. 
```
retry_limit_hw_value = ( retry_limit_raw==0?0:((retry_limit_raw - 1)&0xF) );
```
Override retry_limit_hw_value to 0 to disable re-transmission. Override it to 1 means that let FPGA do maximum 1 time re-transmission.

The FPGA also has a register to override the re-transmission and ACK behavior. Check the current register value.
```
./sdrctl dev sdr0 get reg xpu 11
```
When operate this register, make sure you only change the relevant bits and leave other bits untouched, because other bits have other purposes. Also check the xpu register 11 in the [project document](../README.md)

To override the maximum number of re-transmission, set bit3 to 1, and set the value (0 ~ 7) to bit2 ~ 0. Example, override the maximum number of re-transmission to 1
```
./sdrctl dev sdr0 set reg xpu 11 9
```
9 in binary form is 01001.

To disable the ACK TX after receiving a packet, set bit4 to 1:
```
./sdrctl dev sdr0 set reg xpu 11 16
```
If we want to preserve the above re-transmission overriding setting while disable ACK Tx:
```
./sdrctl dev sdr0 set reg xpu 11 25
```
25 in binary form is 11001. the 1001 of bit3 to 1 is untouched.

Disabling ACK TX might be useful for monitor mode and packet injection.

To disable the ACK RX after sending a packet, set bit5 to 1:
```
./sdrctl dev sdr0 set reg xpu 11 32
```

To disable both ACK Tx and Rx:
```
./sdrctl dev sdr0 set reg xpu 11 48
```
  
## NAV DIFS EIFS CW disable and enable

To check the current NAV/DIFS/EIFS/CW disable status, just run
```
./nav_disable.sh
./difs_disable.sh
./eifs_disable.sh
./cw_disable.sh
```
If NAV is disabled, the openwifi will always assume the NAV (Network Allocation Vector) is already counting down to 0. If DIFS/EIFS is disabled, when the CSMA engine needs to wait for DIFS/EIFS, it won't wait anymore. If CW is disabled, the contention window is fixed to 0, and there won't be any number of slots for random backoff procedure. To disable them, just input 1 as the script argument.
```
./nav_disable.sh 1
./difs_disable.sh 1
./eifs_disable.sh 1
./cw_disable.sh 1
```
To enable them, just input 0 as the script argument.
```
./nav_disable.sh 0
./difs_disable.sh 0
./eifs_disable.sh 0
./cw_disable.sh 0
```

## CW max and min config

When the openwifi NIC bring up (as AP/Client/ad-hoc/etc), Linux will configure the CW (Contention Window) max and min value for FPGA queue 3 ~ 0 via openwifi_conf_tx() in the openwifi driver. You can check the current CW configuration in FPGA (set by Linux).
```
./cw_max_min_cfg.sh
```
It will show sth like
```
FPGA  cw max min for q3 to q0: 1023 15; 63 15; 15 7; 7 3
FPGA  cw max min for q3 to q0: a4644332
```
The CW max and min for q3 ~ 0  are a4, 64, 43, 32 (in hex). Example explanation for q3: in hex the configuration is a4, which means 10 and 4 in the logarithmic domain, (2^10)-1=1023 and (2^4)-1=15 in the linear domain.

To override the CW max and min for queue 3 ~ 0, for example 2047 31; 63 31; 15 7; 7 3, just map it to a hex string b5654332 for queue 3 ~ 0 and give it as the script argument:
```
./cw_max_min_cfg.sh b5654332
```
It will show sth like
```
FPGA  cw max min for q3 to q0: 2047 31; 63 31; 15 7; 7 3
FPGA  cw max min for q3 to q0: b5654332
SYSFS cw max min for q3 to q0: 2047 31; 63 31; 15 7; 7 3
SYSFS cw max min for q3 to q0: b5654332
```
To give the control back to Linux
```
./cw_max_min_cfg.sh 0
```
Be careful that above command won't bring the Linux CW max min setting back to FPGA automatically, because Linux normally only call the setting function openwifi_conf_tx() for 1 time when the NIC is started. So either you write down the Linux setting by checking it at the beginning, and set it back via cw_max_min_cfg.sh before giving it argument 0, or re-load the NIC/driver to trigger the Linux setting action for the NIC.
  
## Rx gain config

In normal operation, you don't need to do Rx gain control manually, because it is controled by the AD9361 AGC function. For optimization/experiment purpose, you might want to use the manual rx gain control, you can run
```
./set_rx_gain_manual.sh 30
```
Above command will turn the automatic gain control mode to manual gain control mode, and set 30dB to the Rx gain module.

Bring it back to the automatic gain control mode
```
./set_rx_gain_auto.sh
```
To find out a good reference about a manual Rx gain setting for the current link/peer, you can set it to automatic mode and then run
```
./stat_enable.sh
```
once and then
```
./rx_gain_show.sh
```
for multiple times to check the actual AGC gain vlaue for received packet as explained in this [Access counter/statistics in driver](driver_stat.md). Then you can set the AGC gain value as argument to the **set_rx_gain_manual.sh** with the corret **offset**! For example, if **rx_gain_show.sh** reports a AGC gain value 34 for many successfully received data packets, and you want to use it as a manual gain setting, you need to set
```
./set_rx_gain_manual.sh 20
```
if the current working channel is 5220MHz (34 - 14dB offset = 20). You need to set
```
./set_rx_gain_manual.sh 29
```
if the current working channel is in 2.4GHz  (34 - 5dB offset = 29). 
  
## Tx power config
```
./sdrctl dev sdr0 set reg rf 0 20000
```
Above command will set Tx power attenuation to 20dB (20*1000). By default it is 0dB.

If you want an initial attenuation 20dB while loading and bringing up the openwifi NIC, please use the **init_tx_att** argument for the sdr.ko.
```
insmod sdr.ko init_tx_att=20000
```
You can change above driver loading action at the end of **wgd.sh**.

The initial Tx attenuation might be useful when you connect two SDR boards directly by cable. Even though, you shouldn't not connect them during the setup phase (bring up the AP or client), because the initialization/tuning of AD9361 might generate big Tx power and kill the other AD9361's Rx. Only connect two SDR boards by cable after both sides have been setup and the attenuation setting takes effect.

To increase the Tx power, you can consider add external PA like [this](https://github.com/open-sdr/openwifi/issues/53#issuecomment-767621478). Or increase the value of register 13 of tx_intf (check [README](../README.md)).

Read the register value:
```
./sdrctl dev sdr0 get reg tx_intf 13
```

Set the register value to N (a number larger than the value read back above):
```
./sdrctl dev sdr0 set reg tx_intf 13 N
```
Bigger value in that register could hurt the Tx EVM and long packet signal. You need to fine tune it for your case.

## Tx Lo and port config

In normal operation, the Tx Lo and RF port are controled by FPGA automatically during signal Tx. To check the current Tx Lo and RF port switch status
```
./set_tx_port.sh
./set_tx_lo.sh
```
Give argument **1** to above scripts to turn them **ON**, **0** for **OFF**.

To turn off automatic Tx Lo control from FPGA and leave Tx Lo always ON:
```
./sdrctl dev sdr0 set reg xpu 13 1
```
  
## Antenna selection
  
By default, the 1st Tx and Rx antennas are used (tx0 and rx0). You can change the tx antenna to tx1 by
```
./sdrctl dev sdr0 set reg drv_tx 4 1
```
Change the tx antenna back to tx0 by
```
./sdrctl dev sdr0 set reg drv_tx 4 0
```
Change the rx antenna to rx1 and rx0 by
```
./sdrctl dev sdr0 set reg drv_rx 4 1
./sdrctl dev sdr0 set reg drv_rx 4 0
```

## Restrict the frequency

Since the AD9361 frequency tuning could generate big unwanted Tx noise, and it could damage the other AD9361 Rx during the test via cable, a restricted frequency can be set to avoid the possible frequency tuning (such as the background scan of Wifi). For example, you want the AD9361 works only in 5220Mhz:
```
./set_restrict_freq.sh 5220
```
Above command will fix the AD9361 in 5220MHz and let driver ignore frequency tuning request other than 5220MHz. The restriction can be removed by:
```
./set_restrict_freq.sh 0
```
To let openwifi work at arbitrary frequency, please check [Let openwifi work at arbitrary frequency](../README.md#let-openwifi-work-at-arbitrary-frequency)

## Receiver sensitivity control

Sometimes too good sensitivity could be a bad thing. WiFi receiver could be "attracted" by many weak signal/packet in the background, and has less "attention" to its real communication target (client/AP). Openwifi has offered a way to make the receiver less sensitive by setting a threshold. When the received signal is lower than this threshold, the receiver will not try to search the WiFi short preamble, i.e. ignore it. For example, if you want to set -70dBm as the threshold, use:
```
./sdrctl dev sdr0 set reg drv_rx 0 70
```

## Tx rate config
  
By default, the Linux rate adaptation algorithm **minstrel_ht** set the packet rate/MCS automatically via openwifi_tx() function.
```
rate_hw_value = ieee80211_get_tx_rate(dev, info)->hw_value;
```
To override the Linux automatic control for non-ht packet
```
./sdrctl dev sdr0 set reg drv_tx 0 N
```
Value N: 0 for Linux auto control; 4 ~ 11 for 6M, 9M, 12M, 18M, 24M, 36M, 48M, 54M.

To override the Linux automatic control for ht packet
```
./sdrctl dev sdr0 set reg drv_tx 1 N
```
Value N: 0 for Linux auto control; 4 ~ 11 for 6.5M, 13M, 19.5M, 26M, 39M, 52M, 58.5M, 65M. By default, the normal GI is used. To use the short GI, you need to add 16 to the target value N.
  
## Arbitrary Tx IQ sample
  
Arbitrary IQ sample can be written to tx_intf and sent for test purposes. Currently maximum 512 samples, which is decided by the FIFO size in tx_iq_intf.v.

To verify this feature, firstly bring up the sdr0 NIC and put it into non-Tx mode, such as monitor mode. Then setup IQ capture in loopback mode, for example FPGA internal. (Check IQ capture App note for more details)
```
insmod side_ch.ko iq_len_init=8187
# Set 100 to register 11. It means the pre trigger length is 100, so we mainly capture IQ after trigger condition is met
./side_ch_ctl wh11d100
# Set 3 to register 8 -- set trigger condition to tx_intf_iq0_non_zero in tx_intf
./side_ch_ctl wh8d3
# Set the loopback mode to FPGA internal
./side_ch_ctl wh5h4
./side_ch_ctl g0
```

Run `python3 iq_capture.py 8187` on the host PC to wait for IQ capture by FPGA. After FPGA hits the IQ capture trigger condition, this host PC Python script will display and save the captured IQ.

Open another ssh session to the board. Make sure you have an arbitrary_iq_gen directory, tx_intf_iq_data_to_sysfs.sh and tx_intf_iq_send.sh (from user_space directory in the host PC) on board. Then run:
```
# Send the example IQ data to driver sysfs, and read back for check
./tx_intf_iq_data_to_sysfs.sh
# Write the IQ data from driver to FPGA, and send once
./tx_intf_iq_send.sh
```

Above scripts will send one time IQ. So you should see the captured IQ plot pop up. You can further check/process iq.txt in Matlab by `test_iq_file_display.m`.

The related tx_intf registers and tx_iq_intf ports
tx_intf register|tx_iq_intf port|explanation
----------------|---------------|-----------
slv_reg7 bit0   |tx_arbitrary_iq_mode| 0:Normal operation; 1:Arbitrary IQ mode
slv_reg7 bit1   |tx_arbitrary_iq_tx_trigger| jumping from 0 to 1 will trigger one time Tx of IQ in the tx_iq_intf FIFO
slv_reg1 bit31~0|tx_arbitrary_iq_in|The driver writes IQ (32bit=I+Q) via this port one by one into the FIFO

The script `tx_intf_iq_send.sh` will trigger driver operations via `tx_intf_iq_ctl_store` function in `sysfs_intf.c`:
- Switch to arbitrary IQ mode via slv_reg7 bit0
- Write IQ data one by one to the tx_intf FIFO via slv_reg1
- Trigger one time Tx of IQ data via slv_reg7 bit1
