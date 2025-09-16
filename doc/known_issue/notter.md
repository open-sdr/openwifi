# Known issue

- [Network issue in quick start](#Network-issue-in-quick-start)
- [EXT4 fs error rootfs issue](#EXT4-fs-error-rootfs-issue)
- [EXT4 fs error rootfs issue while booting on zcu102](#EXT4-fs-error-rootfs-issue-while-booting-on-zcu102)
- [antsdr e200 UART console](#antsdr-e200-UART-console)
- [Client can not get IP](#Client-can-not-get-IP)
- [No space left on device](#No-space-left-on-device)
- [Ping issue due to hostname resolving issue caused by DNS server change](#Ping-issue-due-to-hostname-resolving-issue-caused-by-DNS-server-change)
- [FMCOMMS board eeprom issue causes Linux crash](#FMCOMMS-board-eeprom-issue-causes-Linux-crash)
- [Not booting due to SPI flash](#Not-booting-due-to-SPI-flash)
- [Kernel compiling issue like GCC plugins](#Kernel-compiling-issue-like-GCC-plugins)
- [Missing libidn.so.11 while run boot_bin_gen.sh](#Missing-libidn.so.11-while-run-boot_bin_gen.sh)

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

## EXT4 fs error rootfs issue while booting on zcu102

Issue description: same SD card can boot normally on some zcu102 boards but not on some boards else.

Many reportings on internet (while booting zcu102):
```
Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(179,2)
...
---[ end Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(179,2) ]---
```

Need to add following blocks into the mmc entry or sdhci entry of the zcu102 devicetree:
```
xlnx,has-cd = <0x1>;
xlnx,has-power = <0x0>;
xlnx,has-wp = <0x1>;
disable-wp;
no-1-8-v;
broken-cd;
xlnx,mio-bank = <1>;
/* Do not run SD in HS mode from bootloader */
sdhci-caps-mask = <0 0x200000>;
sdhci-caps = <0 0>;
max-frequency = <19000000>;
```
Suspect the main reason: sdcard speed needs to be limited by above.

Might be due to that the sd card interface degrades and becomes unstable after years.

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

## Not booting due to SPI flash

Before loading content on SD card, the on board SPI flash controls some configurations, such as the kernel file and AD9361 crystal frequency (ad9361_ext_refclk=0x2625a8b). When (suspect) there is an issue, the SPI flash can be restored to default by interrupting booting (hitting enter before Linux loading in the UART console), then
```
Zynq> env default -a
## Resetting to default environment
Zynq> saveenv
Saving Environment to SPI Flash...
SF: Detected n25q256a with page size 256 Bytes, erase size 4 KiB, total 32 MiB
Erasing SPI flash...Writing to SPI flash...done
```

## Kernel compiling issue like GCC plugins

Sometimes after the GNU/GCC tool chain update in the host PC or the slightly kernel update (such as 5.15.0 --> 5.15.36), it might prompt user to select among some new options while compiling kernel like:
```
...
Xen guest support on ARM (XEN) [N/y/?] n
Use a unique stack canary value for each task (STACKPROTECTOR_PER_TASK) [Y/n/?] (NEW) n
*
* GCC plugins
*
GCC plugins (GCC_PLUGINS) [Y/n/?] (NEW) n
...
```
In these cases, the best/safest way is to chose **n** and **weakest** options. Otherwise the compiling might fail or potential issues might happen.

## Missing libidn.so.11 while run boot_bin_gen.sh

You might need to prepare/fake libidn.so.11 by
```
sudo ln -s  /usr/lib/x86_64-linux-gnu/libidn.so.12.6.3 /usr/lib/x86_64-linux-gnu/libidn.so.11
```
Please check/confirm what is the exact **libidn.so.12.6.3** in your system.
