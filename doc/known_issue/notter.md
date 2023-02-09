# Known issue

- [Network issue in quick start](#Network-issue-in-quick-start)
- [antsdr e200 UART console](#antsdr-e200-UART-console)
- [Client can not get IP](#Client-can-not-get-IP)

## Network issue in quick star

- OS: Ubuntu 22 LTS
- image: [openwifi img](https://drive.google.com/file/d/1fb8eJGJAntOciCiGFVLfQs7m7ucRtSWD/view?usp=share_link)

If can't ssh to the board via Ethernet for the 1st time, you might need to delete /etc/network/interfaces.new on SD card (on your computer).

If still can't ssh the board via Ethernet, you should use UART console (/dev/ttyUSBx, /dev/ttyCH341USBx, etc.) to monitor what happened during booting.

## antsdr e200 UART console

If can't see the UART console in Linux (/dev/ttyUSB0 or /dev/ttyCH341USB0), according to https://github.com/juliagoda/CH341SER, you might need to do `sudo apt remove brltty`

## Client can not get IP

If the client can not get IP from the openwifi AP, just re-run "service isc-dhcp-server restart" on board and do re-connect from the client.
