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
  insmod side_ch.ko
  ./side_ch_ctl g
  ```
  You should see on board outputs like:
  ```
  loop 64 side info count 61
  loop 128 side info count 99
  ...
  ```
  If the second number (61, 99, ...) is not zero and keeps increasing, that means the CSI (Chip State Information) is going to the computer smoothly.
  
- Open another terminal on the computer, and run:
  ```
  cd openwifi/user_space/side_ch_ctl_src
  python3 side_info_display.py
  ```
  The python script needs "matplotlib.pyplot" and "numpy" packages installed. Now you should see 3 figures showing run-time **frequency offset**, **channel state/response** and **constellation form equalizer**. Meanwhile the python script prints the **timestamp**.
  
  While running, all informations are also stored into a file side_info.txt. A matlab script **test_side_info_file_display.m** is offered to help you do analysis on the Chip State Information offline.

## Config the capture condition and interval
  The quick start guide will monitor all CSI informations of all packets decoded by the WiFi ofdm receiver. To monitor only specific packets that match the specific FC (Frame Control), addr1 (target MAC address), addr2 (source MAC address), configuration command should be issued before executing "**side_ch_ctl g**". The configuration command is realized by feeding a different parameter to "**side_ch_ctl**". 
  
  A quick example: Capture only CSI of those packets from the device with MAC address 56:5b:01:ec:e2:8f
  ```
  ./side_ch_ctl wh1h4001
  ./side_ch_ctl wh7h01ece28f
  ./side_ch_ctl g
  ```
  The parameter string format is explained in detail:
  ```
  whXhY
  ```
  The X is the register index, and the Y is the value in hex format. The remaining "w", "h" and "h" should be kept untouched.
  - To turn on conditional capture, X should be 1. For Y: bit11~bit0 should be 001(hex), bit12: on/off of FC match, bit13: on/off of addr1 match, bit14 : on/off of addr2 match. Examples:
  ```
  Turn on FC match:
  ./side_ch_ctl wh1h1001
  
  Turn on addr2 (source address) match:
  ./side_ch_ctl wh1h4001
  
  Turn on FC and addr1 (target address) match:
  ./side_ch_ctl wh1h3001
  
  Turn off conditional capture (all packets will be captured):
  ./side_ch_ctl wh1h0001
  ```
  - To specify the condition matching target:
  ```
  Specify the FC matching target:
  ./side_ch_ctl wh5hY
  
  Specify the addr1 (target address) matching target:
  ./side_ch_ctl wh6hY
  
  Specify the addr2 (source address) matching target:
  ./side_ch_ctl wh7hY
  ```  
  The command "**side_ch_ctl g**" will perform CSI capture every 100ms until you press ctrl+C. To use a different capture interval:
  ```
  side_ch_ctl gN
  ```
  The interval will become N*100ms

## Understand CSI feature
  The CSI information is extracted via the openwifi **side channel** infrastructure. This figure explains the related module (and related source code file name) and how the information goes from the board to the computer.
  ![](./csi-architecture.jpg)

  The CSI information format is shown in this figure.
  ![](./csi-information-format.jpg)

  For each element, the actual size is 64bit.
  - timestamp: 64bit TSF timer value, which is the same timestamp value shown by other sniffer software, like tcpdump, wireshark or openwifi printing in dmesg.
  - freq_offset: Only the 1st 16bit is used.
  - csi (channel state/response) and equalizer: Only the first two 16bit are used for I/Q of channel response and equalizer output. The remaining two 16bit are reserved for future multi-antenna cases.
  
  The python and Matlab scripts are recommended for you to understand the CSI packet format precisely.
  
## Config the num_eq
  The num_eq (number of equalizer output) is configurable in case you don't need so many equalizer informations. The valid value is 0~8. You should align the num_eq value at the side_ch.ko, side_info_display.py and test_side_info_file_display.m. 
  - When insert the kernel module, use:
  ```
  insmod side_ch.ko num_eq_init=3
  ```
  You can replace 3 by number 0~8. (8 is the default value. You don't need to specify it like in the Quick start section)
  - When launch the python script, use:
  ```
  side_info_display.py 3
  ```
  - When use the Matlab script, please change the num_eq variable in the script to 3 (3 is just an example).

## Run CSI together with mode other than monitor
  The CSI could run with not only monitor mode. When you run openwifi in AP-Client or ad-hoc mode, after your communication functionality is fully up, you can start from "**insmod side_ch.ko**" and "**./side_ch_ctl g**" on board as described in the quick start section to extract CSI to your computer.

## Map the CSI information to the WiFi packet
  If you want to relate the CSI information to the WiFi packet, you need to capture WiFi packets (tcpdump/wireshark/etc) while capturing CSI. Then you can match the timestamp (TSF timer value) between WiFi packet and CSI information, because this is the unique same identity of a Wifi packet and related CSI.
  
  Please read the python and Matlab script to extract CSI information per packet according to your requirement.
