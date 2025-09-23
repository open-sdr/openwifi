<!--
Author: Xianjun jiao, Michael Mehari, Wei Liu
SPDX-FileCopyrightText: 2019 UGent
SPDX-License-Identifier: AGPL-3.0-or-later
-->

# openwifi
<img src="./openwifi-arch.jpg" width="900">

**openwifi:** Linux mac80211 compatible full-stack IEEE802.11/Wi-Fi design based on SDR (Software Defined Radio).

- We remain committed to open source, which is our foundation. To access advanced features and dedicated support, consider a **SUBSCRIPTION**. More info on https://openwifi.tech

[[Download img and Quick start](#Download-img-and-Quick-start)] [[known issue](doc/known_issue/notter.md)] [[**Tips for Windows users**](https://github.com/open-sdr/openwifi/discussions/341)]

This repository includes Linux driver and software. **openwifi-hw** repository has the FPGA design. It is **YOUR RESPONSIBILITY** to follow your **LOCAL SPECTRUM REGULATION** or use **CABLE** to avoid potential interference over the air.

[[Project document](doc/README.md)]
[[Application notes](doc/app_notes/README.md)]
[[Videos](doc/videos.md)]
[[Publications and How to Cite](doc/publications.md)]
[[maillist](https://lists.ugent.be/wws/subscribe/openwifi)]

Openwifi code has dual licenses. [AGPLv3](https://github.com/open-sdr/openwifi/blob/master/LICENSE) is the opensource license. For non-opensource and advanced feature license, please fill a contact form on https://openwifi.tech. Openwifi project also leverages some 3rd party modules. It is user's duty to check and follow licenses of those modules according to the purpose/usage. You can find [an example explanation from Analog Devices](https://github.com/analogdevicesinc/hdl/blob/master/LICENSE) for this compound license conditions. [[How to contribute]](https://github.com/open-sdr/openwifi/blob/master/CONTRIBUTING.md). 

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
- **802.11ax** and more advanced features, check: https://openwifi.tech

**Performance (best case: aggregation/AMPDU on):**
- iperf: TCP 40~50Mbps; UDP 50Mbps
- EVM -38dB; MCS0 sensitivity -92dBm; MCS7 -73dBm. (FMCOMMS2 2.4GHz; cable and OTA test)

**Supported SDR platforms:**

board_name|Description|Vivado license
----------|-----------|--------------
zc706_fmcs2|[Xilinx ZC706 board](https://www.xilinx.com/products/boards-and-kits/ek-z7-zc706-g.html) + [FMCOMMS2/3/4](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad-fmcomms2.html)|Need
zed_fmcs2|[Xilinx zed board](https://www.xilinx.com/products/boards-and-kits/1-8dyf-11.html) + [FMCOMMS2/3/4](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad-fmcomms2.html)|**NO** need
adrv9364z7020|[ADRV9364-Z7020 + ADRV1CRR-BOB](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/adrv9364-z7020.html)|**NO** need
adrv9361z7035|[ADRV9361-Z7035 + ADRV1CRR-BOB/FMC](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/ADRV9361-Z7035.html)|Need
zc702_fmcs2|[Xilinx ZC702 board](https://www.xilinx.com/products/boards-and-kits/ek-z7-zc702-g.html) + [FMCOMMS2/3/4](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad-fmcomms2.html)|**NO** need
antsdr|[MicroPhase](https://github.com/MicroPhase/) enhanced ADALM-PLUTO [Notes](kernel_boot/boards/antsdr/notes.md)|**NO** need
e310v2|[MicroPhase](https://github.com/MicroPhase/) new antsdr [Notes](kernel_boot/boards/e310v2/README.md)|**NO** need
antsdr_e200|[MicroPhase](https://github.com/MicroPhase/) enhanced ADALM-PLUTO (smaller/cheaper) [Notes](kernel_boot/boards/antsdr_e200/README.md)|**NO** need
sdrpi|[HexSDR](https://github.com/HexSDR/) SDR in Raspberry Pi size [Notes](kernel_boot/boards/sdrpi/notes.md)|**NO** need
zcu102_fmcs2|[Xilinx ZCU102 board](https://www.xilinx.com/products/boards-and-kits/ek-u1-zcu102-g.html) + [FMCOMMS2/3/4](https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/eval-ad-fmcomms2.html)|Need
neptunesdr|Low cost Zynq 7020 + AD9361 board (Unofficial!)|**NO** need
LibreSDR|[Low cost Zynq 7020 + AD9361 board (Unofficial!)](https://github.com/pavelyazev/openwifi-libresdr)|**NO** need

- Check [Porting guide](#Porting-guide) for your new board if it isn't in the list.
- board_name is used to identify FPGA design in openwifi-hw/boards/ and FPGA image in openwifi-hw-img/boards
- Don't have any boards? Or you like JTAG boot instead of SD card? Check our test bed [w-iLab.t](https://doc.ilabt.imec.be/ilabt/wilab/tutorials/openwifi.html) tutorial.

[[Download img and Quick start](#Download-img-and-Quick-start)]
[[Basic operations](#Basic-operations)]
[[Update FPGA](#Update-FPGA)]
[[Update Driver](#Update-Driver)]
[[Update sdrctl](#Update-sdrctl)]
[[Update Misc Helpers](#Update-Misc-Helpers)]

[[Build openwifi Linux img from scratch](#Build-openwifi-Linux-img-from-scratch)]
[[Special note for 11b](#Special-note-for-11b)]
[[Porting guide](#Porting-guide)]
[[Project document](doc/README.md)]
[[Application notes](doc/app_notes/README.md)]

## Download img and Quick start
- Download [openwifi img](https://users.ugent.be/~xjiao/openwifi-1.5.0-shahecheng.img.xz), unzip and burn it into a SD card (>=16GB). After this operation, the SD card should have two partitions: BOOT and rootfs. To flash the SD card, SD card tool software (such as Startup Disk Creator in Ubuntu) or dd command can be used:
  ```
  sudo dd bs=512 count=31116288 if=openwifi-xyz.img of=/dev/your_sdcard_dev
  (To have correct count value, better to check the .img file actual situation by "fdisk -l img_filename")
  ```
- Config the **correct files in the BOOT partition** according to the **board you have** by operation on your computer: 
  - Copy files in **BOOT/openwifi/board_name** to the base directory of BOOT partition.
  - Delete the **rootfs/root/kernel_modules** directory (if exist).
  - Delete the **rootfs/etc/network/interfaces.new** directory (if exist).
- Insert the SD card to the board. Configure the board in SD booting mode. Connect antennas. Power on. 
- Login to the board from your PC (PC Ethernet should have IP 192.168.10.1) with password **openwifi**.
  ```
  ssh root@192.168.10.122
  ```
- If not successful, check [known issue](doc/known_issue/notter.md)
- Then, run openwifi AP and the on board webserver
  ```
  raspi-config --expand-rootfs (Only needed when your SD card > 16GB. Run and reboot)
  ./openwifi/setup_once.sh (Reboot the board. Only need to run once for new board)
  cd openwifi
  ./wgd.sh
  ./fosdem.sh
  (Use "./wgd.sh 1" to enable experimental AMPDU aggregation on top of 11n)
  (Use "./fosdem-11ag.sh" to force 11a/g mode)
  ```
  **NOTE** adrv9361z7035 has ultra low TX power in 5GHz. Move **CLOSER** when you use that board in 5GHz!!!
- After you see the "openwifi" SSID on your device (Phone/Laptop/etc), connect it (If not get 192.168.13.* IP automatically, check [known issue](doc/known_issue/notter.md)). Browser to 192.168.13.1 on your device, you should see the webpage hosted by the webserver on board.
  - Note 1: If your device doesn't support 5GHz (ch44), please change the **hostapd-openwifi.conf** on board and re-run fosdem.sh.
  - Note 2: After ~2 hours, the Viterbi decoder will halt (Xilinx Evaluation License). Just reload FPGA ([method](doc/app_notes/drv_fpga_dynamic_loading.md)) or simply power cycle the board if it happens. (If output of "./sdrctl dev sdr0 get reg rx 20" is always the same, it means the decoder halts)
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

Since the pre-built SD card image might not have the latest bug-fixes/updates, it is recommended to always copy the latest files in the [user_space](./user_space) directory on to the board. Then update the FPGA&Driver according to the Quick start of [this app note](doc/app_notes/radar-self-csi.md#quick-start). Following instructions are doing the same thing with extra info for environment setup.

(Check [Driver and FPGA dynamic reloading app note](./doc/app_notes/drv_fpga_dynamic_loading.md) for better understanding of updating FPGA and driver files without rebooting/power-cycle)

- Install Vivado 2021.1. Make sure install Vitis as well. You should have this directory: your_Xilinx_install_directory/Vitis (NOT Vitis_HLS!)
  - If the Vitis is not installed, you can add it by running "Xilinx Design Tools --> Add Design Tools for Devices 2021.1" from Xilinx program group/menu in your OS start menu.
- Setup environment variables (use absolute path):
  ```
  export XILINX_DIR=your_Xilinx_install_directory
  (Example: export XILINX_DIR=/opt/Xilinx. The Xilinx directory should include sth like: Downloads, Vitis, etc.)
  export OPENWIFI_HW_IMG_DIR=your_openwifi-hw-img_directory
  (The directory where you get the open-sdr/openwifi-hw-img repo via git clone)
  export BOARD_NAME=your_board_name
  ```
- Pick the FPGA bitstream from openwifi-hw-img, generate system_top.bit.bin and transfer it on board via ssh channel:
  ```
  cd openwifi/user_space; ./boot_bin_gen.sh $XILINX_DIR $BOARD_NAME $OPENWIFI_HW_IMG_DIR/boards/$BOARD_NAME/sdk/system_top.xsa
  scp ./system_top.bit.bin root@192.168.10.122:openwifi/
  ```
- Now the system_top.bit.bin is onboard in /root/openwifi/ directory. When wgd.sh runs onboard from that directory, it will discover the FPGA img file system_top.bit.bin and load it before loading driver .ko files.

## Update Driver

Since the pre-built SD card image might not have the latest bug-fixes/updates, it is recommended to always copy the latest files in the [user_space](./user_space) directory on to the board. Then update the FPGA&Driver according to the Quick start of [this app note](doc/app_notes/radar-self-csi.md#quick-start). Following instructions are doing the same thing with extra info for environment setup.

(Check [Driver and FPGA dynamic reloading app note](./doc/app_notes/drv_fpga_dynamic_loading.md) for better understanding of updating FPGA and driver files without rebooting/power-cycle)

- Prepare Analog Devices Linux kernel source code (only need to run once):
  ```
  sudo apt install flex bison libssl-dev device-tree-compiler u-boot-tools -y
  cd openwifi/user_space; ./prepare_kernel.sh $XILINX_DIR ARCH_BIT
  (For Zynq 7000, ARCH_BIT should be 32, for Zynq MPSoC, ARCH_BIT should be 64)
  ```
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
  
  **Note**: If you have symbol or version error while loadng the driver, it could be because the kernel in the SD card image is too old. In this case, you need put the linux kernel image generated by prepare_kernel.sh (check [[Update Driver](#Update-Driver)]) to the BOOT partition of SD card. The kernel image file name: adi-linux/arch/arm/boot/uImage (32bit); adi-linux-64/arch/arm64/boot/Image (64bit).

## Update sdrctl
- Copy the sdrctl source files to the board via ssh channel
  ```
  cd openwifi/user_space/sdrctl_src; scp `find ./ -name \*` root@192.168.10.122:openwifi/sdrctl_src/
  ```
- Compile the sdrctl **on board**:
  ```
  cd ~/openwifi/sdrctl_src/ && make clean && make && cp sdrctl ../ && cd ..
  ```
## Update Misc Helpers

- Check [Driver and FPGA dynamic reloading app note](./doc/app_notes/drv_fpga_dynamic_loading.md) for more convenient way of updating FPGA and driver files without rebooting/power-cycle.
- Update new kernel, modules and devicetree to the board
  - Prepare in the host PC (run scripts in the user_space directory)
    - `prepare_kernel.sh`
    - `boot_bin_gen.sh`
    - `transfer_kernel_image_module_to_board.sh`
  - Run on board (in the /root/ directory)
    - `populate_kernel_image_module_reboot.sh`
    
      If kernel version is changed, you should run this script again after rebooting. Because the first time run it with old kernel will not setup correct liked directory name for the new kernel version.
  - Suggest also update the Linux rootfs (https://wiki.analog.com/resources/tools-software/linux-software/kuiper-linux/update)
    - `git clone https://github.com/analogdevicesinc/linux_image_ADI-scripts.git` on board
    - `apt update`
    - `adi_update_tools.sh`
- Update new drivers .ko files to the board
  - Prepare in the host PC
    - `make_all.sh` (in the driver directory)
    - `transfer_driver_userspace_to_board.sh`
  - Run on board (in the /root/ directory)
    - `populate_driver_userspace.sh`
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

## Build openwifi Linux image from scratch
- For the ADI Kuiper image, please check [kuiper.md](./doc/img_build_instruction/kuiper.md)

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

This section explains the porting work by showing the differences between openwifi and Analog Devices reference design. openwifi is based on 2021_r1 of [HDL Reference Designs](https://github.com/analogdevicesinc/hdl).
- Open the fmcomms2 + zc706 reference design at hdl/projects/fmcomms2/zc706 (Please read Analog Devices help)
- Open the openwifi design zc706_fmcs2 at openwifi-hw/boards/zc706_fmcs2 (Please read openwifi-hw repository)
- "Open Block Design", you will see the differences between openwifi and the reference design. Both in "diagram" and in "Address Editor".
- The address/interrupts of FPGA blocks hooked to the ARM bus should be put/aligned to the devicetree file openwifi/kernel_boot/boards/zc706_fmcs2/devicetree.dts. Linux will parse the devicetree.dtb when booting to know information of attached device (FPGA blocks in our case).
- We use dtc command to get devicetree.dts converted from devicetree.dtb in [Analog Devices Linux image](https://wiki.analog.com/resources/tools-software/linux-software/zynq_images), then do modification according to what we have added/modified to the reference design.
- Please learn the script in [[Build openwifi Linux img from scratch](#Build-openwifi-Linux-img-from-scratch)] to understand how we generate devicetree.dtb, BOOT.BIN, Linux kernel and put them together to build the full SD card image.

## License

This project is available as open source under the terms of the AGPL 3.0 Or later. However, some elements are being licensed under GPL 2-0 or later and BSD 3 license . For accurate information, please check individual files.

## Funding

This project received funding through [ORCA project](https://www.orca-project.eu/). ORCA project is funded by the EU's Horizon2020 programme under agreement number 732174.

This project received funding through [NGI Zero Core](https://nlnet.nl/core/), a fund established by [NLnet](https://nlnet.nl/) with financial support from the European Commission's [Next Generation Internet](https://ngi.eu/) program. Learn more at the NLnet project pages: [802.11n feature of openwifi](https://nlnet.nl/project/OpenWifi-80211n/), [openwifi: 802.11a/g/n maturity](https://nlnet.nl/project/OpenWifi-maturity/), [Extensive openwifi support for OpenWRT](https://nlnet.nl/project/OpenWifi-OpenWRT/)

