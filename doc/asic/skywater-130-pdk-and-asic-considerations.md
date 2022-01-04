Hello,

The skywater PDK and free MPW shuttle are interesting.  And indeed we are asked many times to consider sky130 or other ASIC process MPW.

We do agree that building a real openwifi chip will probably (or not) mean a lot for the community, user and the world.

But, due to our limited bandwidth, currently we are focusing on making the openwifi IP more stable/mature/as-good-as COTS WiFi chip by using the FPGA verification platform, so we haven’t found time to take a look at a real WiFi chip design yet. 

WiFi chips could be as cheap as 0.5USD, but it doesn't mean the WiFi chip is simple. This is contrary to many people’s minds. I tried to explain the complexity of the WiFi chip in some videos, such as the FOSDEM and Libreplanet videos on this page: https://github.com/open-sdr/openwifi/blob/master/doc/videos.md . The WiFi chip is cheap only because they are sold so many per year. From this perspective, the WiFi chip is really an essential tiny thing of the modern world.

But we are definitely glad to support/answer-questions if someone else could jump in and do a solid analysis on the ASIC design effort. Some hints:

1 . The info and communication hub is our github: https://github.com/open-sdr/openwifi . The FPGA code is in https://github.com/open-sdr/openwifi-hw .

2 . The best way to get full picture and further info (resource/power/clock-speed/etc) of the openwifi FPGA design is downloading Xilinx Vivado (version is listed on our github) tool chain, and go through our full FPGA build procedure (README of openwifi-hw: https://github.com/open-sdr/openwifi-hw/blob/master/README.md ), where you will see the full system block diagram: not only the openwifi IP, but also all interfacing/peripheral IP around. Of course many of them are Xilinx/Analog-Devices specific.

3 . You don’t need to pay any fee for Xilinx Vivado, if you chose the FPGA boards (the full list of supported FPGA board is in the README of openwifi: https://github.com/open-sdr/openwifi/blob/master/README.md ) that has 7020 FPGA, because Xilinx offer free offer for that small scale FPGA.

4 . Try to find out all the vendor/3rd-part (Xilinx/Analog-Devices/etc) IPs, and evaluate/estimate how big the efforts will be if they need to be turned into sky130 or other ASIC design. As far as I remember (not full list), inside openwifi IP, we use these IP cores from Xilinx:
- FFT
- Viterbi decoder
- FIFO
- dual port RAM
- ROM
- FIR filter
- AXI stream DMA
- AXI lite bus
- integer divider
- integer multiplexer
- etc. 

I guess most of them need to be ported if we go for a real chip. 

5 . Also outside openwifi IP, there are interfacing/peripheral IPs from Xilinx/Analog-Devices, which can be seen if you create and open the openwifi project block diagram in Vivado (follow the openwifi-hw README). Two special things: RF and ARM processor interconnection. 
- Currently the RF front-end is AD9361 (off-FPGA), which is not a dedicated WiFi front-end (2.4GH/5GHz only). Instead, AD9361 is a quite expensive front-end that supports 70M~6GHz for SDR (Software Defined Radio) applications. So of course, there will be dedicated AD9361 interfacing IPs from Analog Devices (open source as well: https://github.com/analogdevicesinc/hdl, but the license situation is complicated: https://github.com/analogdevicesinc/hdl/blob/master/LICENSE )
- Unlike usual WiFi chips that work with processors via USB/PCIe/SDIO/etc bus, openwifi IP interconnects to the ARM processor via AXI bus. This brings us some unique benefits, such as low latency, but it also makes the IP quite platform dependent. 

6 . Last but not least, considering the efforts (seems big) needed for a real openwifi ASIC, we believe that some bigger/stronger organizations (like foundation/company/person), that have rich experience on IP/licensing analysis and ASIC design, could set up an initiative to work on this openwifi chip activity. Of course, we will be more than happy to join and support it. But to be honest, the openwifi team has very limited ASIC design experiences, and we mainly focus on FPGA for now (due to the bandwidth: personal resource, funding, etc.)

Further discussions/ideas? Feel free to reach out to us!

Best regards,

Xianjun

