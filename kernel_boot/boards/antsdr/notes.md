# antsdr for openwifi-hw

## Introduction
[ANTSDR](https://github.com/MicroPhase/antsdr-fw) is a SDR hardware platform based on [xilinx zynq7020](https://www.xilinx.com/products/silicon-devices/soc/zynq-7000.html) and [adi ad936x](https://www.analog.com/en/products/ad9361.html). It could be used as a traditional SDR device such as PlutoSDR or FMCOMMS2/3/4 with Xilinx evaluation board, and it also be used as hardware platform to support openwifi.

<!--
This README file will give the instructions about how to make the openwifi-hw project for antsdr in the [antsdr branch](https://github.com/MicroPhase/openwifi-hw/tree/antsdr) of openwifi-hw project.
-->
<!--
Above should be unnecessary, because antsdr will be in the master in the future.
-->

## Work to be done
The antsdr has RF switch in the front-end, for now, the RF switch is fixed at a higer range, which will isolation the frequency below 3GHz and pass the frequency at 3GHz~6GHz. 
For future work, it can add the rf swicth control in the devicetree, and this will change the rf switch with the frequency change.
