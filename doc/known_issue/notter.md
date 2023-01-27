# Known issue

- [antsdr e200 uart and network issue](#antsdr-e200-uart-and-network-issue)

## antsdr e200 uart and network issue

- OS: Ubuntu 22 LTS
- Hardware: antsdr e200
- image: [openwifi img](https://drive.google.com/file/d/1fb8eJGJAntOciCiGFVLfQs7m7ucRtSWD/view?usp=share_link)

If can't connect to antsdr e200 for the 1st time, you might need to delete /etc/network/interfaces.new on SD card.

If can't see the UART console in Linux (/dev/ttyCH341USB0), according to https://github.com/juliagoda/CH341SER, you might need to do `sudo apt remove brltty`
