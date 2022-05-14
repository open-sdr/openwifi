<!--
Author: Xianjun jiao, Michael Mehari, Wei Liu
SPDX-FileCopyrightText: 2019 UGent
SPDX-License-Identifier: AGPL-3.0-or-later
-->

# openwifi
<img src="./openwifi-arch.jpg" width="900">

**openwifi:** Linux mac80211 compatible full-stack IEEE802.11/Wi-Fi design based on SDR (Software Defined Radio).

This repository includes Linux driver and software. **openwifi-hw** repository has the FPGA design. It is **YOUR RESPONSIBILITY** to follow your **LOCAL SPECTRUM REGULATION** or use **CABLE** to avoid potential interference over the air.

[[Quick start](#Quick-start)]
[[Project document](doc/README.md)]
[[Application notes](doc/app_notes/README.md)]
[[Videos](doc/videos.md)]
[[Publications and How to Cite](doc/publications.md)]
[[maillist](https://lists.ugent.be/wws/subscribe/openwifi)]

Openwifi code has dual licenses. [AGPLv3](https://github.com/open-sdr/openwifi/blob/master/LICENSE) is the opensource license. For non-opensource and advanced feature license, please contact Filip.Louagie@UGent.be. Openwifi project also leverages some 3rd party modules. It is user's duty to check and follow licenses of those modules according to the purpose/usage. You can find [an example explanation from Analog Devices](https://github.com/analogdevicesinc/hdl/blob/master/LICENSE) for this compound license conditions. [[How to contribute]](https://github.com/open-sdr/openwifi/blob/master/CONTRIBUTING.md). 

**Features:**

- 802.11a/g/n [[IEEE 802.11n (Wi-Fi 4)](doc/app_notes/ieee80211n.md)]
- 20MHz bandwidth; [70 MHz to 6 GHz frequency range](doc/README.md#let-openwifi-work-at-arbitrary-frequency)
- Mode tested: [Ad-hoc](doc/app_notes/ad-hoc-two-sdr.md); [Station; AP](doc/app_notes/ap-client-two-sdr.md), Monitor
- [DCF (CSMA/CA) low MAC layer in FPGA (10us SIFS is achieved)](doc/app_notes/frequent_trick.md)
- [802.11 packet injection and fuzzing](doc/app_notes/inject_80211.md)
- [CSI](doc/app_notes/csi.md): Channel State Information, freq offset, equalizer to computer
- [CSI fuzzer](doc/app_notes/csi_fuzzer.md): Create artificial channel response in WiFi transmitter
- [CSI radar](doc/app_notes/radar-self-csi.md): Moving detection. Joint radar and communication
- [[IQ capture](doc/app_notes/iq.md)]: real-time AGC, RSSI, IQ sample to computer. [[Dual antenna version](doc/app_notes/iq_2ant.md)]
- [Configurable channel access priority parameters](doc/app_notes/frequent_trick.md):
  - CCA threshold, receiver sensitivity, etc
  - duration of RTS/CTS, CTS-to-self
  - SIFS/DIFS/xIFS/slot-time/CW/etc
- [Time slicing based on MAC address (time gated/scheduled FPGA queues)](https://doc.ilabt.imec.be/ilabt/wilab/tutorials/openwifi.html#sdr-tx-time-slicing)
- Easy to change bandwidth and [frequency](doc/README.md#let-openwifi-work-at-arbitrary-frequency): 
  - 2MHz for 802.11ah in sub-GHz
  - 10MHz for 802.11p/vehicle in 5.9GHz
- **802.11ax** under development

**Performance (best case: aggregation/AMPDU on):**
- iperf: TCP 40~50Mbps; UDP 50Mbps
- EVM -38dB; MCS0 sensitivity -87dBm; MCS7 -72dBm. (FMCOMMS2 2.4GHz; cable and OTA test)

**Supported SDR platforms:** (Check [Porting guide](#Porting-guide) for your new board if it isn't in the list)

board_name|board combination|status|SD card img|Vivado license
-------|-------|----|----|-----
zc706_fmcs2|[Xilinx ZC706 board](https://www.xilinx.com/products/boards-and-kits/ek-z7-zc706-g.html) + [FMCOMMS2/3/4](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad-fmcomms2.html)|Done|[32bit img](https://users.ugent.be/~xjiao/openwifi-1.3.0-wilsele-32bit.img.xz)|Need
zed_fmcs2|[Xilinx zed board](https://www.xilinx.com/products/boards-and-kits/1-8dyf-11.html) + [FMCOMMS2/3/4](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad-fmcomms2.html)|Done|[32bit img](https://users.ugent.be/~xjiao/openwifi-1.3.0-wilsele-32bit.img.xz)|**NO** need
adrv9364z7020|[ADRV9364-Z7020 + ADRV1CRR-BOB](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/adrv9364-z7020.html)|Done|[32bit img](https://users.ugent.be/~xjiao/openwifi-1.3.0-wilsele-32bit.img.xz)|**NO** need
adrv9361z7035|[ADRV9361-Z7035 + ADRV1CRR-BOB/FMC](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/ADRV9361-Z7035.html)|Done|[32bit img](https://users.ugent.be/~xjiao/openwifi-1.3.0-wilsele-32bit.img.xz)|Need
zc702_fmcs2|[Xilinx ZC702 board](https://www.xilinx.com/products/boards-and-kits/ek-z7-zc702-g.html) + [FMCOMMS2/3/4](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad-fmcomms2.html)|Done|[32bit img](https://users.ugent.be/~xjiao/openwifi-1.3.0-wilsele-32bit.img.xz)|**NO** need
antsdr|[MicroPhase](https://github.com/MicroPhase/) enhanced ADALM-PLUTO [Notes](kernel_boot/boards/antsdr/notes.md)|Done|[32bit img](https://users.ugent.be/~xjiao/openwifi-1.3.0-wilsele-32bit.img.xz)|**NO** need
zcu102_fmcs2|[Xilinx ZCU102 board](https://www.xilinx.com/products/boards-and-kits/ek-u1-zcu102-g.html) + [FMCOMMS2/3/4](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad-fmcomms2.html)|Done|[64bit img](https://users.ugent.be/~xjiao/openwifi-1.3.0-wilsele-64bit.img.xz)|Need
zcu102_9371|[Xilinx ZCU102 board](https://www.xilinx.com/products/boards-and-kits/ek-u1-zcu102-g.html) + [ADRV9371](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-adrv9371.html)|Future|Future|Need

- board_name is used to identify FPGA design in openwifi-hw/boards/
- Don't have any boards? Or you like JTAG boot instead of SD card? Check our test bed [w-iLab.t](https://doc.ilabt.imec.be/ilabt/wilab/tutorials/openwifi.html) tutorial.

[[Quick start](#Quick-start)]
[[Basic operations](#Basic-operations)]
[[Update FPGA](#Update-FPGA)]
[[Update Driver](#Update-Driver)]
[[Update sdrctl](#Update-sdrctl)]
[[Easy Access and etc](#Easy-Access-and-etc)]

[[Build openwifi Linux img from scratch](#Build-openwifi-Linux-img-from-scratch)]
[[Special note for 11b](#Special-note-for-11b)]
[[Porting guide](#Porting-guide)]
[[Project document](doc/README.md)]
[[Application notes](doc/app_notes/README.md)]

## Quick start
- Restore openwifi board specific img file (from the table) into a SD card. To do this, program "Disks" in Ubuntu can be used (Install: "sudo apt install gnome-disk-utility"). After restoring, the SD card should have two partitions: BOOT and rootfs. You need to config the **correct files in the BOOT partition** according to the **board you have** by operation on your computer: 
  - Copy files in **openwifi/board_name** to the base directory of BOOT partition.
  - Copy **openwifi/zynqmp-common/Image** (zcu102 board) or **openwifi/zynq-common/uImage** (other boards) to the base directory of BOOT partition
- Connect two antennas to RXA/TXA ports. Config the board to SD card boot mode (check the board manual). Insert the SD card to the board. Power on. 
- Login to the board from your PC (PC Ethernet should have IP 192.168.10.1) with password **openwifi**.
  ```
  ssh root@192.168.10.122
  ```
- Then, run openwifi AP and the on board webserver
  ```
  cd openwifi
  ./wgd.sh
  ./fosdem.sh
  (Use "./wgd.sh 1" to enable experimental AMPDU aggregation on top of 11n)
  (Use "./fosdem-11ag.sh" to force 11a/g mode)
  ```
  **NOTE** adrv9361z7035 has ultra low TX power in 5GHz. Move **CLOSER** when you use that board in 5GHz!!!
- After you see the "openwifi" SSID on your device (Phone/Laptop/etc), connect it. Browser to 192.168.13.1 on your device, you should see the webpage hosted by the webserver on board.
  - Note 1: If your device doesn't support 5GHz (ch44), please change the **hostapd-openwifi.conf** on board and re-run fosdem.sh.
  - Note 2: After ~2 hours, the Viterbi decoder will halt (Xilinx Evaluation License). Just power cycle the board if it happens. (If output of "./sdrctl dev sdr0 get reg rx 20" is always the same, it means the decoder halts)
- To give the Wi-Fi client internet access, configure routing/NAT **on the PC**:
  ```
  sudo sysctl -w net.ipv4.ip_forward=1
  sudo iptables -t nat -A POSTROUTING -o NICY -j MASQUERADE
  sudo ip route add 192.168.13.0/24 via 192.168.10.122 dev ethX
  ```
  **ethX** is the PC NIC name connecting the board ethernet. **NICY** is the PC NIC name connecting internet (WiFi or another ethernet).
  
  If you want, uncommenting "net.ipv4.ip_forward=1" in /etc/sysctl.conf to make IP forwarding persistent on PC.
- To monitor **real-time CSI (Chip State Information)**, such as timestamp, frequency offset, channel state, equalizer, please refer to [[CSI notes](doc/app_notes/csi.md)].

## Basic operations
The board actually is an Linux/Ubuntu computer which is running **hostapd** to offer Wi-Fi AP functionality over the Wi-Fi Network Interface (NIC). The NIC is implemented by openwifi-hw FPGA design. We use the term **"On board"** to indicate that the commands should be executed after ssh login to the board. **"On PC"** means the commands should run on PC.
- Bring up the openwifi NIC sdr0:
  ```
  cd ~/openwifi && ./wgd.sh
  (Use "./wgd.sh 1" to enable experimental AMPDU aggregation)
  ```
- Use openwifi as client to connect other AP (Change wpa-connect.conf on board firstly):
  ```
  route del default gw 192.168.10.1
  wpa_supplicant -i sdr0 -c wpa-connect.conf &
  dhclient sdr0
  ```
- Use openwifi in ad-hoc mode: Please check **sdr-ad-hoc-up.sh**, **sdr-ad-hoc-join.sh** and [this app note](./doc/app_notes/ad-hoc-two-sdr.md).
- Use openwifi in monitor mode: Please check **monitor_ch.sh** and [this app note](./doc/app_notes/inject_80211.md).
- The Linux native Wi-Fi tools/Apps (iwconfig/ifconfig/iwlist/iw/hostapd/wpa_supplicant/etc) can run over openwifi NIC in the same way as commercial Wi-Fi chip. 
- **sdrctl** is a dedicated tool to access openwifi driver/FPGA, please check [project document](./doc/README.md) for more information. 

## Update FPGA

(Check [Driver and FPGA dynamic reloading app note](./doc/app_notes/drv_fpga_dynamic_loading.md) for more convenient way of updating FPGA and driver files)

Since the pre-built SD card image might not have the latest bug-fixes/updates, it is recommended to always copy the latest files in the [user_space](./user_space) directory on to the board. Then update the fpga bitstream and driver (see next section) on to the board.

- Install Vivado/SDK 2018.3 (Vivado Design Suite - HLx Editions - 2018.3 Full Product Installation. If you don't need to generate new FPGA bitstream, WebPack version without license is enough)
- Setup environment variables (use absolute path):
  ```
  export XILINX_DIR=your_Xilinx_install_directory
  (Example: export XILINX_DIR=/opt/Xilinx. The Xilinx directory should include sth like: Downloads, SDK, Vivado, xic)
  export OPENWIFI_HW_DIR=your_openwifi-hw_directory
  (The directory where you store the open-sdr/openwifi-hw repo via git clone)
  export BOARD_NAME=your_board_name
  ```
- Pick the FPGA bitstream from openwifi-hw, and generate BOOT.BIN and transfer it on board via ssh channel:
  ```
  For Zynq 7000:
  
  cd openwifi/user_space; ./boot_bin_gen.sh $OPENWIFI_HW_DIR $XILINX_DIR $BOARD_NAME
  
  For Zynq MPSoC (like zcu102 board):
  cd openwifi/user_space; ./boot_bin_gen_zynqmp.sh $OPENWIFI_HW_DIR $XILINX_DIR $BOARD_NAME
  
  cd openwifi/kernel_boot/boards/$BOARD_NAME/output_boot_bin; scp ./BOOT.BIN root@192.168.10.122:
  ```
- On board: Put the BOOT.BIN into the BOOT partition.
  ```
  mount /dev/mmcblk0p1 /mnt
  cp ~/BOOT.BIN /mnt
  cd /mnt
  sync
  cd ~
  umount /mnt
  ```
  **Power cycle** the board to load new FPGA bitstream.
  
  To load FPGA dynamically without rebooting/power-cycle, check [Driver and FPGA dynamic reloading app note](./doc/app_notes/drv_fpga_dynamic_loading.md).

## Update Driver

(Check [Driver and FPGA dynamic reloading app note](./doc/app_notes/drv_fpga_dynamic_loading.md) for more convenient way of updating FPGA and driver files)

Since the pre-built SD card image might not have the latest bug-fixes/updates, it is recommended to always copy the latest files in the [user_space](./user_space) directory on to the board. Then update the fpga bitstream (see previous section) and driver on to the board.

- Prepare Analog Devices Linux kernel source code (only need to run once):
  ```
  cd openwifi/user_space; ./prepare_kernel.sh $XILINX_DIR ARCH_BIT build
  (For Zynq 7000, ARCH_BIT should be 32, for Zynq MPSoC, ARCH_BIT should be 64)
  ```
  **Note**: In Ubuntu, gcc-10 might have issue ('yylloc' error), so use gcc-9 if you encounter error.
- Compile the latest openwifi driver
  ```
  cd openwifi/driver; ./make_all.sh $XILINX_DIR ARCH_BIT
  (For Zynq 7000, ARCH_BIT should be 32, for Zynq MPSoC, ARCH_BIT should be 64)
  (More arguments (max 5) beyond above two will be converted to "#define argument" in pre_def.h for conditional compiling)
  ```
- Copy the driver files to the board via ssh channel
  ```
  cd openwifi/driver; scp `find ./ -name \*.ko` root@192.168.10.122:openwifi/
  ```
  Now you can use **wgd.sh** on board to load the new openwifi driver. **wgd.sh** also tries to reload FPGA img if system_top.bit.bin presents in the same directory. 
  Find more information in [Driver and FPGA dynamic reloading app note](./doc/app_notes/drv_fpga_dynamic_loading.md).
  
  **Note**: If you have symbol or version error while loadng the driver, it could be because the kernel in the SD card image is too old. In this case, you need to follow [[Build openwifi Linux img from scratch](#Build-openwifi-Linux-img-from-scratch)] to generate your new SD card image.

## Update sdrctl
- Copy the sdrctl source files to the board via ssh channel
  ```
  cd openwifi/user_space/sdrctl_src; scp `find ./ -name \*` root@192.168.10.122:openwifi/sdrctl_src/
  ```
- Compile the sdrctl **on board**:
  ```
  cd ~/openwifi/sdrctl_src/ && make && cp sdrctl ../ && cd ..
  ```
## Easy Access and etc

- Check [Driver and FPGA dynamic reloading app note](./doc/app_notes/drv_fpga_dynamic_loading.md) for more convenient way of updating FPGA and driver files.
- FPGA and driver on board update scripts
  - Setup [ftp server](https://ubuntu.com/server/docs/service-ftp) on PC, allow anonymous and change ftp root directory to the openwifi directory.
  - On board:
  ```
  ./sdcard_boot_update.sh $BOARD_NAME
  (Above command downloads uImage, BOOT.BIN and devicetree.dtb, then copy them into boot partition. Remember to power cycle)
  ./wgd.sh remote
  (Above command downloads driver files, and brings up sdr0)
  ```
- Access the board disk/rootfs like a disk: 
   - On PC: "File manager --> Connect to Server...", input: sftp://root@192.168.10.122/root
   - Input password "openwifi"

## Build openwifi Linux img from scratch
- Install the devicetree compiler -- dtc. (For Ubuntu: sudo apt install device-tree-compiler)
- Install the mkimage tool. (For Ubuntu: sudo apt install u-boot-tools)
- Download [2019_R1-2020_06_22.img.xz](http://swdownloads.analog.com/cse/2019_R1-2020_06_22.img.xz) from [Analog Devices Wiki](https://wiki.analog.com/resources/tools-software/linux-software/zynq_images). Burn it to a SD card.
- Insert the SD card to your Linux PC. Find out the mount point (that has two sub directories BOOT and rootfs), and setup environment variables (use absolute path):
  ```
  export SDCARD_DIR=sdcard_mount_point
  export XILINX_DIR=your_Xilinx_install_directory
  export OPENWIFI_HW_DIR=your_openwifi-hw_directory
  export BOARD_NAME=your_board_name
  ```
- Run script to update SD card:
  ```
  cd openwifi/user_space; ./update_sdcard.sh $OPENWIFI_HW_DIR $XILINX_DIR $BOARD_NAME $SDCARD_DIR
  ```
- Config your board to SD card boot mode (check the board manual). Insert the SD card to the board. Power on.
- Login to the board from your PC (PC Ethernet should have IP 192.168.10.1) with one time password **analog**.
  ```
  ssh root@192.168.10.122
  ```
- Setup routing/NAT **on the PC** for your board -- this internet connection is **important** for post installation/config.
  ```
  sudo sysctl -w net.ipv4.ip_forward=1
  sudo iptables -t nat -A POSTROUTING -o NICY -j MASQUERADE
  sudo ip route add 192.168.13.0/24 via 192.168.10.122 dev ethX
  ```
  **ethX** is the PC NIC name connecting the board ethernet. **NICY** is the PC NIC name connecting internet (WiFi or another ethernet).
  
  If you want, uncommenting "net.ipv4.ip_forward=1" in /etc/sysctl.conf to make IP forwarding persistent on PC.
- Run **one time** script on board to complete post installation/config (After this, password becomes **openwifi**)
  ```
  cd ~/openwifi && ./post_config.sh
  ```
- Now you can start from [Quick start](#Quick-start) (Skip the image download and burn step)

## Special note for 11b

Openwifi only applies OFDM as its modulation scheme and as a result, it is not backward compatible with 802.11b clients or modes of operation. This is usually the case during beacon transmission, connection establishment, and robust communication.

As a solution to this problem, openwifi can be fully controlled only if communicating with APs/clients instantiated using hostapd/wpa_supplicant userspace programs respectively.

For hostapd program, 802.11b rates can be suppressed using configuration commands (i.e. supported_rates, basic_rates) and an example configuration file is provided (i.e. hostapd-openwifi.conf). One small caveat to this one comes from fullMAC Wi-Fi cards as they must implement the *NL80211_TXRATE_LEGACY* NetLink handler at the device driver level.

On the other hand, the wpa_supplicant program on the client side (commercial Wi-Fi dongle/board) cannot suppress 802.11b rates out of the box in 2.4GHz band, so there will be an issue when connecting openwifi (OFDM only). A patched wpa_supplicant should be used at the client side.
```
sudo apt-get install libssl1.0-dev
cd openwifi/user_space; ./build_wpa_supplicant_wo11b.sh
```
## Porting guide

This section explains the porting work by showing the differences between openwifi and Analog Devices reference design. openwifi is based on 2019_R1 of [HDL Reference Designs](https://github.com/analogdevicesinc/hdl).
- Open the fmcomms2 + zc706 reference design at hdl/projects/fmcomms2/zc706 (Please read Analog Devices help)
- Open the openwifi design zc706_fmcs2 at openwifi-hw/boards/zc706_fmcs2 (Please read openwifi-hw repository)
- "Open Block Design", you will see the differences between openwifi and the reference design. Both in "diagram" and in "Address Editor".
- The address/interrupts of FPGA blocks hooked to the ARM bus should be put/aligned to the devicetree file openwifi/kernel_boot/boards/zc706_fmcs2/devicetree.dts. Linux will parse the devicetree.dtb when booting to know information of attached device (FPGA blocks in our case).
- We use dtc command to get devicetree.dts converted from devicetree.dtb in [Analog Devices Linux image](https://wiki.analog.com/resources/tools-software/linux-software/zynq_images), then do modification according to what we have added/modified to the reference design.
- Please learn the script in [[Build openwifi Linux img from scratch](#Build-openwifi-Linux-img-from-scratch)] to understand how we generate devicetree.dtb, BOOT.BIN and Linux kernel uImage and put them together to build the full SD card image.

## License

This project is available as open source under the terms of the AGPL 3.0 Or later. However, some elements are being licensed under GPL 2-0 or later and BSD 3 license . For accurate information, please check individual files.
