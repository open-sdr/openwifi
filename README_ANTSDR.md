# antsdr for openwifi

## Introduction
[ANTSDR](https://github.com/MicroPhase/antsdr-fw) is a SDR hardware platform based on [xilinx zynq7020](https://www.xilinx.com/products/silicon-devices/soc/zynq-7000.html) and [adi ad936x](https://www.analog.com/en/products/ad9361.html). It could be used as a traditional SDR device such as PlutoSDR or FMCOMMS2/3/4 with Xilinx evaluation board, and it also be used as hardware platform to support openwifi.
This README file will give the instructions about how to make the openwifi image for antsdr in the [antsdr branch](https://github.com/MicroPhase/openwifi/tree/antsdr) of openwifi project.


## Build openwifi Linux img from scratch
- To build the openwifi image, you should install the Xilinx Vivado 2018.3 first.
- Download [2019_R1-2020_02_04.img.xz](https://swdownloads.analog.com/cse/2019_R1-2020_02_04.img.xz) from [Analog Devices Wiki](https://wiki.analog.com/resources/tools-software/linux-software/zynq_images). Burn it to a SD card.
    ```bash
    sudo dd if= 2019_R1-2020_02_04.img of=/dev/sdb bs=1M
    ```
    The **sdb** is your sd card device, this could be different with your environment. 
- Clone the opwnwifi project and checkoout to antsdr branch.
  ```bash
  git clone https://github.com/MicroPhase/openwifi.git
  git checkout antsdr
  ```
- Insert the SD card to your Linux PC. Find out the mount point (that has two sub directories BOOT and rootfs), and setup environment variables (use absolute path):
  ```bash
  export SDCARD_DIR=sdcard_mount_point
  export XILINX_DIR=your_Xilinx_directory
  export OPENWIFI_DIR=your_openwifi_directory
  export BOARD_NAME=your_board_name
  ```
  For example:
  ```bash
  export SDCARD_DIR=/media/wcc/
  export XILINX_DIR=/opt/Xilinx/
  export OPENWIFI_DIR=/home/wcc/openwifi
  export BOARD_NAME=antsdr
  ```
- Run script to update SD card:
  ```bash
  $OPENWIFI_DIR/user_space/update_sdcard.sh $OPENWIFI_DIR $XILINX_DIR $BOARD_NAME $SDCARD_DIR
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

  For example:
  ```
  sudo sysctl -w net.ipv4.ip_forward=1
  sudo iptables -t nat -A POSTROUTING -o wlan0 -j MASQUERADE
  sudo ip route add 192.168.13.0/24 via 192.168.10.122 dev eth0
  ```
  
  If you want, uncommenting "net.ipv4.ip_forward=1" in /etc/sysctl.conf to make IP forwarding persistent on PC.
- Run **one time** script on board to complete post installation/config (After this, password becomes **openwifi**)
  ```
  cd ~/openwifi && ./post_config.sh
  ```
- Now you can start from [Quick start](https://github.com/MicroPhase/openwifi/tree/antsdr#quick-start) (Skip the image download and burn step)