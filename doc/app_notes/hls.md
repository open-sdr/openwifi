<!--
Author: Thijs Havinga
SPDX-FileCopyrightText: 2019 UGent
SPDX-License-Identifier: AGPL-3.0-or-later
-->

FCCM2023 Poster: [Thijs Havinga, et al. Accelerating FPGA-Based Wi-Fi Transceiver Design and Prototyping by High-Level Synthesis](https://github.com/open-sdr/openwifi-hw-img/raw/master/doc_repo/Thijs-FCCM2023-poster.jpg)

[Longer/detailed info about the poster](https://arxiv.org/abs/2305.13351)

In order to speed up or ease FPGA development, it is possible to use High-Level Synthesis (HLS) for creating core baseband processing modules of openwifi. We have already programmed the receiver modules channel estimation and equalization in C++ and converted to Verilog using Vitis HLS. In order to use openwifi with these HLS modules, follow the [build instructions](#build-instructions).
In order to modify these modules within Vitis HLS, follow [the instructions below](#modify-the-code-using-vitis-hls). 

## Build instructions

Follow the [Build FPGA](https://github.com/open-sdr/openwifi-hw#build-fpga) instructions till before generating ip_repo. In order to switch to the HLS-version of openofdm_rx, use the following commands:

```
cd ip/openofdm_rx
git checkout dot11zynq_hls
```

Now continue with the instructions. Before generating the bitstream, update the openofdm_rx IP by making sure it is selected under "IP Status" and click "Upgrade Selected". Afterwards, continue with the instructions to generate the bitstream.

## Modify the code using Vitis HLS
When in the `openwifi-hw` folder, make sure to run:
```
./get_ip_openofdm_rx.sh
cd ip/openofdm_rx
git checkout dot11zynq_hls
```
Then start Vitis HLS and create a new project. Import either all source files (except those ending on '_test.cpp') in the [ch_gain_cal](https://github.com/open-sdr/openofdm/tree/dot11zynq_hls/hls/ch_gain_cal) or [equalizer](https://github.com/open-sdr/openofdm/tree/dot11zynq_hls/hls/equalizer) folder to modify the channel estimation or equalizer module, respectively. Choose either 'equalizer' or 'ch_gain_cal' as top-level module. Next, select `equalizer_test.cpp` or `ch_gain_cal_test.cpp` as testbench file. In 'Part selection', select the right part corresponding to your board. 

After modifying the code and making sure C simulation and cosimulation is running fine, select 'Export RTL', which will generate a ZIP file with a folder `hdl/verilog` containing the generated Verilog files. Replace the current folder `openwifi-hw/ip/openofdm_rx/hls/equalizer/hdl/verilog/` (or `.../ch_gain_cal/hdl/verilog`) with this folder and change the `openofdm_rx.tcl` file to include the newly generated Verilog files. See [here](https://github.com/open-sdr/openofdm/blob/dot11zynq_hls/openofdm_rx.tcl#L268) for an example. If you modified the top-level function arguments, you will need to interface them accordingly in [dot11.v](https://github.com/open-sdr/openofdm/blob/dot11zynq_hls/verilog/dot11.v).

Now follow the [Build FPGA](https://github.com/open-sdr/openwifi-hw#build-fpga) instructions, starting at the step "Generate ip_repo for the top level FPGA project". It will then use the modified .tcl file to include the correct files for your modified HLS module and build the FPGA using it.

A similar approach can be followed to create other HLS modules, where you would need to execute these steps in the folder of the IP to be modified and integrate the modules in the corresponding top-level Verilog file.
