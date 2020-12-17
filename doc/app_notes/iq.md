We implement the **IQ sample capture** with interesting extensions: many **trigger conditions**; **RSSI**, RF chip **AGC** **status (lock/unlock)** and **gain**.

## Quick start
- Power on the SDR board.
- Connect a computer to the SDR board via Ethernet cable. The computer should have static IP 192.168.10.1. Open a terminal on the computer, and then in the terminal:
  ```
  ssh root@192.168.10.122
  (password: openwifi)
  cd openwifi
  ./wgd.sh
  (Wait for the script completed)
  ./monitor_ch.sh sdr0 11
  (Monitor on channel 11. You can change 11 to other channel that is busy)
  insmod side_ch.ko iq_len_init=8187
  (for smaller FPGA (7Z020), iq_len_init should be <4096, like 4095, instead of 8187)
  
  ./side_ch_ctl wh11d4094
  (Above command is needed only when you run with zed, adrv9364z7020, zc702 board)
  
  ./side_ch_ctl g
  ```
  You should see on board outputs like:
  ```
  loop 64 side info count 61
  loop 128 side info count 99
  ...
  ```
  If the second number (side info count 61, 99, ...) keeps increasing, that means the trigger condition is met from time to time and the IQ sample is going to the computer smoothly.
  
- Open another terminal on the computer, and run:
  ```
  cd openwifi/user_space/side_ch_ctl_src
  python3 iq_capture.py
  (for zed, adrv9364z7020, zc702 board, add argument that euqals to iq_len_init, like 4095)
  ```
  The python script needs "matplotlib.pyplot" and "numpy" packages installed. Now you should see 3 figures showing run-time **IQ sample**, **AGC gain and lock status** and **RSSI (uncalibrated)**. Meanwhile the python script prints the **timestamp**.
  
  While running, all informations are also stored into a file **iq.txt**. A matlab script **test_iq_file_display.m** is offered to help you do analysis on the IQ Information offline. For zed, adrv9364z7020, zc702 board, do not forget to change the **iq_len** in the matlab script to 4095.

## Understand the IQ capture feature
  The IQ information is extracted via the openwifi **side channel** infrastructure. This figure explains the related modules (also related source code file name) and how the information goes from the SDR board to the computer.
  ![](./iq-architecture.jpg)

  The IQ information format is shown in this figure.
  ![](./iq-information-format.jpg)

  For each element, the actual size is 64bit.
  - timestamp: 64bit TSF timer value when the capture is triggered.
  - IQ
    - The first two 16bit are used for I/Q sample from the antenna currently used
    - The 3rd 16bit is AD9361 AGC gain (bit7 -- lock/unlock; bit6~0 -- gain value)
    - The 4th 16bit is RSSI (half dB, uncalibrated). Please check xpu.v and sdr.c to understand how the raw RSSI value is finally calibrated and reported to Linux mac80211.
    
  The python and Matlab scripts are recommended for you to understand the IQ packet format precisely.

## Config the IQ capture and interval
  The quick start guide captures a period of history IQ when the packet FCS checksum is checked by Wifi receiver (no matter pass or fail). To initiate the capture with different trigger conditions and length, configuration commands should be issued before executing "**side_ch_ctl g**". The configuration command is realized by feeding a different parameter to "**side_ch_ctl**". The main parameters that are configurable are explained in this figure.
  ![](./iq-capture-parameter.jpg)
  
  **iq_len** is the number of IQ samples captured per trigger condition met. The capture is started from the time **pre_trigger_len** IQ samples before the trigger moment. **iq_len** is set only one time when you insert the side_ch.ko. Please check the next section for **iq_len** configuration. This section introduces the setting of pre_trigger_len and trigger condition.
  - pre_trigger_len
  ```
  ./side_ch_ctl wh11dY
  ```
  The parameter **Y** specifies the pre_trigger_len. Valid range 0 ~ 8190. It is limited by the FPGA fifo size. For **small FPGA** (zed_fmcs2, adrv9364z7020, zc702), the valid range is 0 ~ **4094**.
  - trigger condition
  ```
  ./side_ch_ctl wh8dY
  ```
  The parameter **Y** specifies the trigger condition. Valid range 0 ~ 31, which is explained in this table.
  
  value|meaning
  -----|-------
  0 |receiver gives FCS checksum result. no matter pass/fail
  1 |receiver gives FCS checksum result. pass
  2 |receiver gives FCS checksum result. fail
  3 |receiver gives SIGNAL field checksum result. no matter pass/fail
  4 |receiver gives SIGNAL field checksum result. pass
  5 |receiver gives SIGNAL field checksum result. fail
  6 |receiver gives SIGNAL field checksum result. no matter pass/fail. HT packet
  7 |receiver gives SIGNAL field checksum result. no matter pass/fail. non-HT packet
  8 |receiver gives  long preamble detected
  9 |receiver gives short preamble detected
  10|RSSI (half dB uncalibrated) goes above the threshold
  11|RSSI (half dB uncalibrated) goes below the threshold
  12|AD9361 AGC from lock to unlock
  13|AD9361 AGC from unlock to lock
  14|AD9361 AGC gain goes above the threshold
  15|AD9361 AGC gain goes below the threshold
  16|phy_tx_started signal from openofdm tx core
  17|phy_tx_done signal from openofdm tx core
  18|positive edge of tx_bb_is_ongoing from xpu core
  19|negative edge of tx_bb_is_ongoing from xpu core
  20|positive edge of tx_rf_is_ongoing from xpu core
  21|negative edge of tx_rf_is_ongoing from xpu core
  22|phy_tx_started and this tx packet needs ACK
  23|phy_tx_done and this tx packet needs ACK
  24|positive edge of tx_bb_is_ongoing and this tx packet needs ACK
  25|negative edge of tx_bb_is_ongoing and this tx packet needs ACK
  26|positive edge of tx_rf_is_ongoing and this tx packet needs ACK
  27|negative edge of tx_rf_is_ongoing and this tx packet needs ACK
  28|tx_bb_is_ongoing and I/Q amplitude from the other antenna is above rssi_or_iq_th
  29|tx_rf_is_ongoing and I/Q amplitude from the other antenna is above rssi_or_iq_th
  30|start tx, meanwhile I/Q amplitude from the other antenna is above rssi_or_iq_th
  31|start tx and need for ACK, meanwhile I/Q amplitude from the other antenna is above rssi_or_iq_th
  
  To set the RSSI threshold
  ```
  ./side_ch_ctl wh9dY
  ```
  The parameter **Y** specifies the RSSI threshold. Valid range 0 ~ 2047.
  
  To set the AGC gain threshold
  ```
  ./side_ch_ctl wh10dY
  ```
  The parameter **Y** specifies the AGC gain threshold. Valid range 0 ~ 127.
  
  The command "**side_ch_ctl g**" will perform IQ capture every 100ms until you press ctrl+C. To use a different capture interval:
  ```
  side_ch_ctl gN
  ```
  The interval will become N*1ms

## Config the iq_len
  The **iq_len** (number of IQ sample per capture) is configurable in case you want less IQ samples per capture so that it can be triggered more times during a specific analysis period. The valid value is 1~**8187**. For **small FPGA** (zed_fmcs2, adrv9364z7020, zc702), the valid range is 0 ~ **4095**. It is independant from pre_trigger_len, and it can be less than pre_trigger_len if you want. You should align the **iq_len** value at the side_ch.ko, iq_capture.py and test_iq_file_display.m. 
  - When insert the kernel module, use:
  ```
  insmod side_ch.ko iq_len_init=3000
  ```
  Here 3000 is an example. **ATTENTION:** You need to specify **iq_len_init** explicitly to turn on IQ capture, which will turn off the default CSI mode. Insert the side_ch.ko without any parameter will run the default CSI mode.
  - When launch the python script, use:
  ```
  python3 iq_capture.py 3000
  ```
  - When use the matlab script, please change the **iq_len** variable in the script to 3000.

## Compile the side channel driver and user space program
  - side_ch.ko
  ```
  $OPENWIFI_DIR/driver/side_ch/make_driver.sh $OPENWIFI_DIR $XILINX_DIR ARCH_BIT
(For Zynq 7000, ARCH_BIT should be 32, for Zynq MPSoC, ARCH_BIT should be 64)
  ```
  - side_ch_ctl (take user_space/side_ch_ctl_src/side_ch_ctl.c and compile it on board!)
  ```
  gcc -o side_ch_ctl side_ch_ctl.c
  ```

## Run the IQ capture together with modes other than monitor
  The openwifi IQ capture feature could run with not only monitor mode but also other modes, such as AP-Client or ad-hoc mode. After the communication functionality is fully up in those modes, you can start IQ capture from "**insmod side_ch.ko**" and "**./side_ch_ctl g**" on board as described in the previous sections to extract IQ information to your computer.

## Map the IQ information to the WiFi packet
  If you want to relate the IQ information to the WiFi packet, you need to capture WiFi packets (tcpdump/wireshark/etc) while capturing IQ. Then you can relate the timestamp between WiFi packet and IQ information. Please be noticed that the timestamp in the IQ information is the moment when capture is triggered, which could be different from the timestamp reported in the packet capture program. But since they share the same time base (TSF timer), you can relate them easily by analyzing the WiFi packet and IQ sample sequence.
  
  Please learn the python and Matlab script to extract IQ information per capture according to your requirement.
