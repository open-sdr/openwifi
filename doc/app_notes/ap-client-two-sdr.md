<!--
SPDX-FileCopyrightText: 2019 Jiao Xianjun <putaoshu@msn.com>
SPDX-License-Identifier: AGPL-3.0-or-later
-->

- Power on two SDR boards. Call one board "AP board" and the other "client board". On each board, the TX and RX antenna should vertical/orthogonal to each other as much as possible to gain a good TX/RX isolation.
- Connect a computer to the AP board via Ethernet cable. The computer should have static IP 192.168.10.1. Open a terminal on the computer, and then in the terminal:
  ```
  ssh root@192.168.10.122
  (password: openwifi)
  cd openwifi
  ./fosdem.sh
  (It will create a WiFi AP by hostapd program with config file: hostapd-openwifi.conf)
  (Wait for the script completed)
  cat /proc/interrupts
  (Execute the "cat ..." command for several times)
  (You should see the number of "sdr,tx_itrpt1" grows, because it sends the "openwifi" beacon periodically)
  ```
- Connect another computer to the client board via Ethernet cable. The computer should have static IP 192.168.10.1. Open a terminal on the computer, and then in the terminal:
  ```
  ssh root@192.168.10.122
  (password: openwifi)
  service network-manager stop
  cd openwifi
  ./wgd.sh
  (Wait for the script completed)
  ifconfig sdr0 up
  iwlist sdr0 scan
  (The "openwifi" AP should be listed in the scanning results)
  iwconfig sdr0 essid openwifi
  ```
- Now the client is trying to associate with the AP. The AP board terminal should print like:
  ```
  ...
  sdr0: AP-STA-CONNECTED 66:55:44:33:22:58
  sdr0: STA 66:55:44:33:22:58 RADIUS: starting accounting session 1FF1C1B4-00000001
  ```
  If not, please adjust antenna/distance and re-run the commands on the client side.

- After association is done, in the terminal of client:
  ```
  dhclient sdr0
  (Wait for it completed)
  ifconfig sdr0
  (Now you should see the IP address like 192.168.13.x allocated by AP)
  ./set_csma_normal.sh
  ping 192.168.13.1
  (Ping the AP)
  ```
  Now the communication link should be already setup between the AP and the client.
