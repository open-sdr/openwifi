**IMPORTANT pre-conditions**:
- Install Vivado 2021.1. Make sure install Vitis as well. You should have this directory: your_Xilinx_install_directory/Vitis (NOT Vitis_HLS!)
  - If the Vitis is not installed, you can add it by running "Xilinx Design Tools --> Add Design Tools for Devices 2021.1" from Xilinx program group/menu in your OS start menu, or Help menu of Vivado.
- SD card at least with 16GB
- Install the devicetree compiler -- dtc. (For Ubuntu: sudo apt install device-tree-compiler)
- Install the mkimage tool. (For Ubuntu: sudo apt install u-boot-tools)

[[Use openwifi prebuilt img](#Use-openwifi-prebuilt-img)]
[[Build SD card from scratch](#Build-SD-card-from-scratch)]
[[Use existing SD card on new board](#Use-existing-SD-card-on-new-board)]

## Use openwifi prebuilt img

Download openwifi pre-built img, such as openwifi-xyz.img.xz, and extract it to .img file.

Use dd command to flash the SD card. (Other software seems having issue!)
```
sudo dd bs=512 count=31116288 if=openwifi-xyz.img of=/dev/your_sdcard_dev
```

(To have correct count value, better to check the .img file actual situation by "fdisk -l img_filename".

## Build SD card from scratch

Download image_2022-08-04-ADI-Kuiper-full.zip from https://wiki.analog.com/resources/tools-software/linux-software/kuiper-linux?redirect=1

Extract it to .img file.

Use dd command to flash the SD card. (Other software seems having issue!)
```
sudo dd bs=512 count=24018944 if=2022-08-04-ADI-Kuiper-full.img of=/dev/your_sdcard_dev
```

(To have correct count value, better to check the .img file actual situation by "fdisk -l img_filename". While making .img from SD card, check the SD card dev instead)

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

Build and put the following 3 files to the BOOT partition of the SD card:
- Linux kernel image file (check how to generate it by prepare_kernel.sh in [Update Driver](../../README.md#Update-Driver)): 
  - adi-linux-64/arch/arm64/boot/Image (64bit)
  - adi-linux/arch/arm/boot/uImage (32bit)
- devicetree file:
  - openwifi/kernel_boot/boards/zcu102_fmcs2/system.dtb (64bit)
  - openwifi/kernel_boot/boards/$BOARD_NAME/devicetree.dtb (32bit)
- BOOT.BIN (check how to generate it by boot_bin_gen.sh in [Update FPGA](../../README.md#Update-FPGA)):
  - openwifi/kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN

Build openwifi driver according to [Update Driver](../../README.md#Update-Driver)). No need to copy them onboard at this moment.

Create /root/openwifi directory in the rootfs partition of the SD card, and put all files in openwifi/user_space to that directory.

Power on the board with the SD card, connect the board to your host PC (static IP 192.168.10.1) via ethernet, and ssh to the board with password "analog"
```
ssh root@192.168.10.122
```

Then change password to "openwifi" via "passwd" command onbard.

Enlarge the onboard SD disk space, and reboot (https://github.com/analogdevicesinc/adi-kuiper-gen/releases)
```
raspi-config --expand-rootfs
reboot now
```

(You should already build Linux kernel and openwifi driver in previous steps!)

Transfer the kernel modules .ko and openwifi driver .ko onto the board:
```
cd openwifi/user_space
./transfer_driver_userspace_to_board.sh
./transfer_kernel_image_module_to_board.sh $LINUX_KERNEL_SRC $BOARD_NAME
($LINUX_KERNEL_SRC is the directory openwifi/adi-linux-64 or adi-linux)
```

In the ssh session onboard:
```
cd /root
sync
tar -zxvf openwifi.tar.gz
./populate_kernel_image_module_reboot.sh
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
cd /root/openwifi
route add default gw 192.168.10.1 || true
ping google.com
sudo apt update
chmod +x *.sh

# build sdrctl and inject_80211
sudo apt-get -y install libnl-3-dev
sudo apt-get -y install libnl-genl-3-dev
cd sdrctl_src
make
cp sdrctl ../
cd ../side_ch_ctl_src/
gcc -o side_ch_ctl side_ch_ctl.c
cp side_ch_ctl ../
cd ..
cd ./inject_80211/
make
cd ..

# install and setup dhcp server
sudo apt-get -y install isc-dhcp-server
cp dhcpd.conf /etc/dhcp/dhcpd.conf

# install hostapd and other useful tools
sudo apt-get -y install hostapd
sudo apt-get -y install tcpdump
sudo apt-get -y install webfs
sudo apt-get -y install iperf
sudo apt-get -y install iperf3
sudo apt-get -y install libpcap-dev
sudo apt-get -y install bridge-utils

# add video file into the onbard web server
wget -P webserver/ https://users.ugent.be/~xjiao/openwifi-low-aac.mp4
```

Run openwifi in the ssh session onboard:
```
cd /root/openwifi
./wgd.sh
iwlist sdr0 scan
./fosdem.sh
```

## Use existing SD card on new board

You don't need to do it from scratch on a blank SD card. Instead, you can start from existing SD card (for example zcu102), and use it for a new board (for example zedboard).

1. Do this section (see above) "Build and put the following 3 files to the BOOT partition of the SD card" for the new board by replacing the file on the existing SD card.

2. Build openwifi driver according to [Update Driver](../../README.md#Update-Driver)). No need to copy them onboard at this moment.

3. Use the existing SD card to boot the new board.

4. Do this section (see above) "Transfer the kernel modules .ko and openwifi driver .ko onto the board". Do not forget the operations onboard after the file transfer:
  ```
  cd /root
  sync
  tar -zxvf openwifi.tar.gz
  ./populate_kernel_image_module_reboot.sh
  reboot now
  ```
