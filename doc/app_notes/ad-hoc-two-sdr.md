<!--
Author: Xianjun jiao
SPDX-FileCopyrightText: 2019 UGent
SPDX-License-Identifier: AGPL-3.0-or-later
-->

**NOTE** the terminal session mentioned in the following text can also be setup via USB-UART instead of Ethernet.

**NOTE** adrv9361z7035 has ultra low TX power in 5GHz. Move **CLOSER** when you use that board in 5GHz!!!

- Power on two SDR boards. Call one board "adhoc1" and the other "adhoc2". On each board, the TX and RX antenna should vertical/orthogonal to each other as much as possible to gain a good TX/RX isolation.
- Connect a computer to the adhoc1 via Ethernet cable. The computer should have static IP 192.168.10.1. Open a terminal on the computer, and then in the terminal:
  ```
  ssh root@192.168.10.122
  (password: openwifi)
  service network-manager stop
  cd openwifi
  ./wgd.sh
  (Wait for the script completed)
  ifconfig sdr0 up
  ./sdr-ad-hoc-up.sh sdr0 44 192.168.13.1
  (Above command setup ad-hoc network at channel 44 with static IP assigned to sdr0 NIC)
  iwconfig sdr0
  ```
- You should see output like:
  ```
  sdr0    IEEE 802.11  ESSID:"sdr-ad-hoc"  
          Mode:Ad-Hoc  Frequency:5.22 GHz  Cell: 92:CA:14:27:1E:B0   
          Tx-Power=20 dBm   
          Retry short limit:7   RTS thr:off   Fragment thr:off
          Encryption key:off
          Power Management:off
  ```
  If you see "Cell: Not-Associated", please wait and run "iwconfig sdr0" again until a randomly generated Cell ID appears.

- Connect another computer to the adhoc2 via Ethernet cable. The computer should have static IP 192.168.10.1. Open a terminal on the computer, and then in the terminal:
  ```
  ssh root@192.168.10.122
  (password: openwifi)
  service network-manager stop
  cd openwifi
  ./wgd.sh
  ifconfig sdr0 up
  ./sdr-ad-hoc-up.sh sdr0 44 192.168.13.2
  iwconfig sdr0
  ```
- You should see output like:
  ```
  sdr0    IEEE 802.11  ESSID:"sdr-ad-hoc"  
          Mode:Ad-Hoc  Frequency:5.22 GHz  Cell: 92:CA:14:27:1E:B0   
          Tx-Power=20 dBm   
          Retry short limit:7   RTS thr:off   Fragment thr:off
          Encryption key:off
          Power Management:off
  ```
  The "Cell: 92:CA:14:27:1E:B0" should be the same as adhoc1, because the later joined node should discover the Cell ID of the existing network and join/get it automatically. If not, please adjust the antenna/distance and re-run the commands.

  Now the communication link should be already setup between the two ad-hoc nodes, and you can ping each other.
