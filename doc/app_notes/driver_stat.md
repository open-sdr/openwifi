Comprehensive statistics are offered at the driver level via the [Linux sysfs](https://en.wikipedia.org/wiki/Sysfs#:~:text=sysfs%20is%20a%20pseudo%20file,user%20space%20through%20virtual%20files.).

[[Quick start](#Quick-start)]
[[Sysfs explanation](#Sysfs-explanation)]
[[Statistics variable file meaning](#Statistics-variable-file-meaning)]

All operations should be done on board in openwifi directory, not in host PC.

## Quick start

Enable the driver level statistics (after openwifi up and running)
```
./stat_enable.sh
```
Show the statistics
```
./tx_stat_show.sh
./tx_prio_queue_show.sh
./rx_stat_show.sh
./rx_gain_show.sh
```
Clear the stattistics
```
./tx_stat_show.sh clear
./tx_prio_queue_show.sh clear
./rx_stat_show.sh clear
```
Let rx_stat_show.sh calculate PER (Packet Error Rate) by giving the number of packet sent at the peer (30000 packets for example):
```
./rx_stat_show.sh 30000
```
To only show the statistics for the link with a specific peer node
```
./set_rx_target_sender_mac_addr.sh c83caf93
(If the peer node MAC address is 00:80:c8:3c:af:93)
```
To show the statistics of all (not filtered by the peer node MAC address)
```
./set_rx_target_sender_mac_addr.sh 0
```
To show the peer node MAC address for statistics
```
./set_rx_target_sender_mac_addr.sh
```
To see the statistics of ACK packet, run this before above scripts
```
./set_rx_monitor_all.sh
```
Disable the statistics of ACK packet, run this before above scripts
```
./set_rx_monitor_all.sh 0
```
Disable the driver level statistics (after openwifi up and running)
```
./stat_enable.sh 0
```

## Sysfs explanation

For user, as you can check in those scripts above, the sysfs is a set of files that can be operated in the command line for communicating with kernel module. You can find these files on zcu102 board at
```
/sys/devices/platform/fpga-axi@0/fpga-axi@0:sdr
```
On othe boards at
```
/sys/devices/soc0/fpga-axi@0/fpga-axi@0:sdr
```

## Statistics variable file meaning

These statistics names are the same as the file names (in those scripts) and variable names in the sdr.c. Do search these names in sdr.c to understand exact meaning of these statistics.

- tx_stat_show.sh

  name|meaning
  ------------|----------------------
  tx_data_pkt_need_ack_num_total     | number of tx data packet reported in openwifi_tx_interrupt() (both fail and succeed)
  tx_data_pkt_need_ack_num_total_fail| number of tx data packet reported in openwifi_tx_interrupt() (fail -- no ACK received)
  tx_data_pkt_need_ack_num_retx      | number of tx data packet reported in openwifi_tx_interrupt() at different number of retransmission (both fail and succeed)
  tx_data_pkt_need_ack_num_retx_fail | number of tx data packet reported in openwifi_tx_interrupt() at different number of retransmission (fail -- no ACK received)
  tx_data_pkt_mcs_realtime           | MCS (10*Mbps) of tx data packet reported in openwifi_tx_interrupt() (both fail and succeed)
  tx_data_pkt_fail_mcs_realtime      | MCS (10*Mbps) of tx data packet reported in openwifi_tx_interrupt() (fail -- no ACK received)
  tx_mgmt_pkt_need_ack_num_total     | number of tx management packet reported in openwifi_tx_interrupt() (both fail and succeed)
  tx_mgmt_pkt_need_ack_num_total_fail| number of tx management packet reported in openwifi_tx_interrupt() (fail -- no ACK received)
  tx_mgmt_pkt_need_ack_num_retx      | number of tx management packet reported in openwifi_tx_interrupt() at different number of retransmission (both fail and succeed)
  tx_mgmt_pkt_need_ack_num_retx_fail | number of tx management packet reported in openwifi_tx_interrupt() at different number of retransmission (fail -- no ACK received)
  tx_mgmt_pkt_mcs_realtime           | MCS (10*Mbps) of tx management packet reported in openwifi_tx_interrupt() (both fail and succeed)
  tx_mgmt_pkt_fail_mcs_realtime      | MCS (10*Mbps) of tx management packet reported in openwifi_tx_interrupt() (fail -- no ACK received)

- tx_prio_queue_show.sh

  tx_prio_queue_show.sh will show 4 rows. Each row is corresponding one Linux-prio and one FPGA queue. Each row has 12 elements. Elements' name will not be displayed in the command line.

  Element name|meaning
  ------------|----------------------
  tx_prio_num            | number of tx packet from Linux prio N to openwifi_tx()
  tx_prio_interrupt_num  | number of tx packet from Linux prio N recorded in openwifi_tx_interrupt()
  tx_prio_stop0_fake_num | number of Linux prio N stopped attempt in the 1st place of openwfii_tx(), fake alarm
  tx_prio_stop0_real_num | number of Linux prio N stopped attempt in the 1st place of openwfii_tx(), real stop
  tx_prio_stop1_num      | number of Linux prio N stopped in the 2nd place of openwfii_tx()
  tx_prio_wakeup_num     | number of Linux prio N waked up in openwifi_tx_interrupt()
  tx_queue_num           | number of tx packet for FPGA queue N to openwifi_tx()
  tx_queue_interrupt_num | number of tx packet for FPGA queue N recorded in openwifi_tx_interrupt()
  tx_queue_stop0_fake_num| number of FPGA queue N stopped attempt in the 1st place of openwfii_tx(), fake alarm
  tx_queue_stop0_real_num| number of FPGA queue N stopped attempt in the 1st place of openwfii_tx(), real stop
  tx_queue_stop1_num     | number of FPGA queue N stopped in the 2nd place of openwfii_tx()
  tx_queue_wakeup_num    | number of FPGA queue N waked up in openwifi_tx_interrupt()

- rx_stat_show.sh

  name|meaning
  ------------|----------------------
  rx_data_pkt_num_total               | number of rx data packet with both FCS ok and failed
  rx_data_pkt_num_fail                | number of rx data packet with FCS failed
  rx_mgmt_pkt_num_total               | number of rx management packet with both FCS ok and failed
  rx_mgmt_pkt_num_fail                | number of rx management packet with FCS failed
  rx_ack_pkt_num_total                | number of rx ACK packet with both FCS ok and failed
  rx_ack_pkt_num_fail                 | number of rx ACK packet with FCS failed
  rx_data_pkt_mcs_realtime            | MCS (10*Mbps) of rx data packet with both FCS ok and failed
  rx_data_pkt_fail_mcs_realtime       | MCS (10*Mbps) of rx data packet with FCS failed
  rx_mgmt_pkt_mcs_realtime            | MCS (10*Mbps) of rx management packet with both FCS ok and failed
  rx_mgmt_pkt_fail_mcs_realtime       | MCS (10*Mbps) of rx management packet with FCS failed
  rx_ack_pkt_mcs_realtime             | MCS (10*Mbps) of rx ACK packet with both FCS ok and failed
  rx_data_ok_agc_gain_value_realtime  | agc gain value of rx data packet with FCS ok
  rx_data_fail_agc_gain_value_realtime| agc gain value of rx data packet with FCS failed
  rx_mgmt_ok_agc_gain_value_realtime  | agc gain value of rx management packet with FCS ok
  rx_mgmt_fail_agc_gain_value_realtime| agc gain value of rx management packet with FCS failed
  rx_ack_ok_agc_gain_value_realtime   | agc gain value of rx ACK packet with FCS ok

- rx_gain_show.sh

  name|meaning
  ------------|----------------------
  rx_data_ok_agc_gain_value_realtime  | agc gain value of rx data packet with FCS ok
  rx_data_fail_agc_gain_value_realtime| agc gain value of rx data packet with FCS failed
  rx_mgmt_ok_agc_gain_value_realtime  | agc gain value of rx management packet with FCS ok
  rx_mgmt_fail_agc_gain_value_realtime| agc gain value of rx management packet with FCS failed
  rx_ack_ok_agc_gain_value_realtime   | agc gain value of rx ACK packet with FCS ok
  
  Note: gain value here is always 14 dB higher than set_rx_gain_auto.sh/set_rx_gain_manual.sh at 5220MHz. 5dB higher at 2.4GHz.
