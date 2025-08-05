**IMPORTANT pre-conditions**:
- Install Vivado 2022.2. Make sure install Vitis as well. You should have this directory: your_Xilinx_install_directory/Vitis (NOT Vitis_HLS!)
  - If the Vitis is not installed, you can add it by running "Xilinx Design Tools --> Add Design Tools for Devices 2022.2" from Xilinx program group/menu in your OS start menu, or Help menu of Vivado.
- SD card at least with 16GB
- Install packages: `sudo apt install flex bison libssl-dev device-tree-compiler u-boot-tools -y`

[[Use openwifi prebuilt img](#Use-openwifi-prebuilt-img)]
[[Build SD card from scratch](#Build-SD-card-from-scratch)]
[[Use existing SD card on new board](#Use-existing-SD-card-on-new-board)]

## Use openwifi prebuilt img

Download openwifi pre-built img (see [Quick start](../../README.md#quick-start)), and extract it to .img file.

Use dd command to flash the SD card. (Or other software like Startup Disk Creator in Ubuntu)
```
sudo dd bs=512 count=31116288 if=openwifi-xyz.img of=/dev/your_sdcard_dev
```
To have correct count value, better to check the .img file actual situation by "fdisk -l img_filename" (check the number of sectors).

Then start from the 2nd step of the [Quick start](../../README.md#quick-start) in README.

## Build SD card from scratch

Download "13 December 2023 release (2022_r2)" (image_2023-12-13-ADI-Kuiper-full.zip) from https://wiki.analog.com/resources/tools-software/linux-software/kuiper-linux?redirect=1

Extract it to .img file.

Use dd command to flash the SD card. (Or other software like Startup Disk Creator in Ubuntu)
```
sudo dd bs=512 count=24182784 if=2023-12-13-ADI-Kuiper-full.img of=/dev/your_sdcard_dev
```

(To have correct count value, better to check the .img file actual situation by "fdisk -l img_filename" (check the number of sectors). While making .img from SD card, check the SD card dev instead)

Mount the BOOT and rootfs partition of SD card to your computer.

Change the SD card file: Add following into rootfs/etc/network/interfaces
```
# The loopback interface
auto lo
iface lo inet loopback
auto eth0
iface eth0 inet static

#your static IP
address 192.168.10.122

#your gateway IP
gateway 192.168.10.1
netmask 255.255.255.0

#your network address "family"
network 192.168.10.0
broadcast 192.168.10.255
```

Change the SD card file: Add following into rootfs/etc/sysctl.conf
```
net.ipv4.ip_forward=1
```

Change the SD card file: Add following into rootfs/etc/systemd/system.conf
```
DefaultTimeoutStopSec=2s
```

Put the openwifi/kernel_boot/10-network-device.rules into rootfs/etc/udev/rules.d/

Run **update_sdcard.sh** from openwifi/user_space directory to further prepare the SD card. The last argument $SDCARD_DIR of the script is the directory (mounting point) on your computer that has BOOT and rootfs directories/partitions.

The script will build and put following things into the SD card:
  - Linux kernel image file ([Update Driver](../../README.md#Update-Driver)): 
    - adi-linux-64/arch/arm64/boot/Image (64bit)
    - adi-linux/arch/arm/boot/uImage (32bit)
  - devicetree file:
    - openwifi/kernel_boot/boards/zcu102_fmcs2/system.dtb (64bit)
    - openwifi/kernel_boot/boards/$BOARD_NAME/devicetree.dtb (32bit)
  - BOOT.BIN ([Update FPGA](../../README.md#Update-FPGA)):
    - openwifi/kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN
  - openwifi driver ([Update Driver](../../README.md#Update-Driver)).
  - openwifi/user_space files and openwifi/webserver files

After **update_sdcard.sh** finishes, please do the 2nd step "Config the correct files ..." in [Quick start](../../README.md#quick-start). Then power on the board with the SD card, connect the board to your host PC (static IP 192.168.10.1) via ethernet, and ssh to the board with password **"analog"**
```
ssh root@192.168.10.122
```

Then change password to "openwifi" via "passwd" command onbard.

Enlarge the onboard SD disk space, and reboot (https://github.com/analogdevicesinc/adi-kuiper-gen/releases)
```
raspi-config --expand-rootfs
reboot now
```
Setup routing/NAT **on the PC** for your board -- this internet connection is **important** for post installation/config.
```
sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -A POSTROUTING -o NICY -j MASQUERADE
sudo ip route add 192.168.13.0/24 via 192.168.10.122 dev ethX
```
**ethX** is the PC NIC name connecting the board ethernet. **NICY** is the PC NIC name connecting internet (WiFi or another ethernet).

If you want, uncommenting "net.ipv4.ip_forward=1" in /etc/sysctl.conf to make IP forwarding persistent on PC.

Test the connectivity. Run on board (in the ssh session):
```
route add default gw 192.168.10.1
ping IP_YOU_KNOW_ON_YOUR_NETWORK
```
If there is issue with the connectivity (ping can not reach the target), it needs to be solved before going to the next step.

Do misc configurations/installations in the ssh session onboard:
```
(You might need to set correct data and time by: date -s)
sudo apt update
chmod +x /root/openwifi/*.sh

# install and setup dhcp server
sudo apt-get -y install isc-dhcp-server
cp /root/openwifi/dhcpd.conf /etc/dhcp/dhcpd.conf

# install hostapd and other useful tools
sudo apt-get -y install hostapd
sudo apt-get -y install tcpdump
sudo apt-get -y install webfs
sudo apt-get -y install iperf
sudo apt-get -y install iperf3
sudo apt-get -y install libpcap-dev
sudo apt-get -y install bridge-utils

# build on board tools
sudo apt-get -y install libnl-3-dev
sudo apt-get -y install libnl-genl-3-dev
cd /root/openwifi/sdrctl_src
make clean
make
cp sdrctl ../
cd /root/openwifi/side_ch_ctl_src/
gcc -o side_ch_ctl side_ch_ctl.c
cp side_ch_ctl ../
cd /root/openwifi/inject_80211/
make clean
make
cd ..
```

Run openwifi in the ssh session onboard:
```
/root/openwifi/setup_once.sh (Only need to run once for new board)
cd /root/openwifi
./wgd.sh
ifconfig sdr0 up
iwlist sdr0 scan
./fosdem.sh
```

## Use existing SD card on new board

Just operate the existing/working SD card of the old board on your computer starting from the 2nd step of the [Quick start](../../README.md#quick-start) in README. Then start using the SD card on the new board.
