# Known issue

- [Network issue in quick start](#Network-issue-in-quick-start)
- [EXT4 fs error rootfs issue](#EXT4-fs-error-rootfs-issue)
- [antsdr e200 UART console](#antsdr-e200-UART-console)
- [Client can not get IP](#Client-can-not-get-IP)
- [No space left on device](#No-space-left-on-device)
- [Ping issue due to hostname resolving issue caused by DNS server change](#Ping-issue-due-to-hostname-resolving-issue-caused-by-DNS-server-change)
- [FMCOMMS board eeprom issue causes Linux crash](#FMCOMMS-board-eeprom-issue-causes-Linux-crash)

## Network issue in quick star

- OS: Ubuntu 22 LTS
- image: [openwifi img](https://drive.google.com/file/d/1fb8eJGJAntOciCiGFVLfQs7m7ucRtSWD/view?usp=share_link)

If can't ssh to the board via Ethernet for the 1st time, you might need to delete /etc/network/interfaces.new on SD card (on your computer).

If still can't ssh the board via Ethernet, you should use UART console (/dev/ttyUSBx, /dev/ttyCH341USBx, etc.) to monitor what happened during booting.

## EXT4 fs error rootfs issue

Sometimes, the 1st booting after flashing SD card might encounter "EXT4-fs error (device mmcblk0p2): ..." error on neptunesdr, changing SD card flashing tool might solve this issue. Some tool candidates:
- gnome-disks
- Startup Disk Creator
- win32diskimager

## antsdr e200 UART console

If can't see the UART console in Linux (/dev/ttyUSB0 or /dev/ttyCH341USB0), according to https://github.com/juliagoda/CH341SER, you might need to do `sudo apt remove brltty`

## Client can not get IP

If the client can not get IP from the openwifi AP, just re-run "service isc-dhcp-server restart" on board and do re-connect from the client.

## No space left on device
It might be due to too many dmesg/log/journal, disk becomes full. 
```
systemd-journald[5694]: Failed to open system journal: No space left on device
```
You can try following operations.
```
systemd-tmpfiles --clean
sudo systemd-tmpfiles --remove
rm /var/log/* -rf
apt --autoremove purge rsyslog
```
Add followings into `/etc/systemd/journald.conf`
```
SystemMaxUse=64M
Storage=volatile
RuntimeMaxUse=64M
ForwardToConsole=no
ForwardToWall=no
```

## Ping issue due to hostname resolving issue caused by DNS server change

You might need to change nameserver to 8.8.8.8 in /etc/resolv.conf on board.

## FMCOMMS board eeprom issue causes Linux crash

Some FMCOMMS2/3/4/x boards shipped with wrong/empty eeprom, so that on some platform (like ZCU102) it causes issues like Linux crash. You can follow https://github.com/analogdevicesinc/fru_tools to reprogram the eeprom.
- Insert the FMCOMMS board on a platform (such as 32bit zed/zc706/zc702/etc) that can boot and boot into Linux
- On board Linux:
  ```
  git clone https://github.com/analogdevicesinc/fru_tools.git
  cd fru_tools/
  make
  find /sys -name eeprom
  (It might return like: /sys/devices/soc0/fpga-axi@0/41620000.i2c/i2c-0/0-0050/eeprom)
  fru-dump -i /sys/devices/soc0/fpga-axi@0/41620000.i2c/i2c-0/0-0050/eeprom -b
  ```
- If there is issue, you will see some "mismatch" warning like:
  ```
  read 256 bytes from /sys/devices/soc0/fpga-axi@0/41620000.i2c/i2c-0/0-0050/eeprom
  fru_dump 0.8.1.7, built 04Aug2022
  FRU Version number mismatch 0xff should be 0x01
  ```
- To reprogram the eeprom (FMCOMMS4 as an example):
  ```
  fru-dump -i ./masterfiles/AD-FMCOMMS4-EBZ-FRU.bin -o /sys/devices/soc0/fpga-axi@0/41620000.i2c/i2c-0/0-0050/eeprom
  ```
- Reboot the board, and try to read eeprom again, correct information should be shown like:
  ```
  fru-dump -i /sys/devices/soc0/fpga-axi@0/41620000.i2c/i2c-0/0-0050/eeprom -b
  read 256 bytes from /sys/devices/soc0/fpga-axi@0/41620000.i2c/i2c-0/0-0050/eeprom
  Date of Man	: Mon Jul 22 20:23:00 2013
  Manufacturer	: Analog Devices
  Product Name	: AD9364 RF Eval/Software Dev Kit
  Serial Number	: 00045
  Part Number	: AD-FMCOMMS4-EBZ
  FRU File ID	: Empty Field
  PCB Rev 	: C
  PCB ID  	: FMCOMMSFMC04A
  BOM Rev 	: 1
  ```
