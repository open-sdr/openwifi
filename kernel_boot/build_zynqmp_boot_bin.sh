#!/bin/bash

# SPDX-FileCopyrightText: 2019 Jiao Xianjun <putaoshu@msn.com>
# SPDX-License-Identifier: AGPL-3.0-or-later
# https://wiki.analog.com/resources/eval/user-guides/ad-fmcomms2-ebz/software/linux/zynqmp

set -ex

HDF_FILE=$1
UBOOT_FILE=$2
ATF_FILE=${3:-download}
BUILD_DIR=build_boot_bin
OUTPUT_DIR=output_boot_bin

usage () {
	echo "usage: $0 system_top.hdf u-boot.elf  (download | bl31.elf | <path-to-arm-trusted-firmware-source>) [output-archive]"
	exit 1
}

depends () {
	echo "Xilinx $1 must be installed and in your PATH"
	echo "try: source /opt/Xilinx/Vivado/201x.x/settings64.sh"
	exit 1
}

### Check command line parameters
echo $HDF_FILE | grep -q ".hdf" || usage
echo $UBOOT_FILE | grep -q -e ".elf" -e "uboot" || usage

if [ ! -f $HDF_FILE ]; then
    echo $HDF_FILE: File not found!
    usage
fi

if [ ! -f $UBOOT_FILE ]; then
    echo $UBOOT_FILE: File not found!
    usage
fi

### Check for required Xilinx tools
command -v xsdk >/dev/null 2>&1 || depends xsdk
command -v bootgen >/dev/null 2>&1 || depends bootgen
command -v hsi >/dev/null 2>&1 || depends hsi

rm -Rf $BUILD_DIR $OUTPUT_DIR
mkdir -p $OUTPUT_DIR
mkdir -p $BUILD_DIR

# 2017.4 use 47af34b94a52b8cdc8abbac44b6f3ffab33a2206
# 2018.1 use df4a7e97d57494c7d79de51b1e0e450d982cea98
# 2018.2 use 93a69a5a3bc318027da4af5911124537f4907642
# 2018.3 use 08560c36ea5b6f48b962cb4bd9a79b35bb3d95ce

hsi_ver=$(hsi -version | head -1 | cut -d' ' -f2)
if [ -z "$hsi_ver" ] ; then
	echo "Could not determine Vivado version"
	exit 1
fi
atf_version=xilinx-$hsi_ver

### Check if ATF_FILE is .elf or path to arm-trusted-firmware
if [ "$ATF_FILE" != "" ] && [ -d $ATF_FILE ]; then
### Build arm-trusted-firmware bl31.elf
(
	cd $ATF_FILE
	make distclean
	git checkout $atf_version
	make CROSS_COMPILE=aarch64-linux-gnu- PLAT=zynqmp RESET_TO_BL31=1
)
	cp $ATF_FILE/build/zynqmp/release/bl31/bl31.elf $OUTPUT_DIR/bl31.elf
elif [ "$ATF_FILE" == "download" ]; then
(
	command -v git >/dev/null 2>&1 || depends git
	cd $BUILD_DIR
	git clone https://github.com/Xilinx/arm-trusted-firmware.git
	cd arm-trusted-firmware
	git checkout $atf_version
	make CROSS_COMPILE=aarch64-linux-gnu- PLAT=zynqmp RESET_TO_BL31=1
)
	cp $BUILD_DIR/arm-trusted-firmware/build/zynqmp/release/bl31/bl31.elf $OUTPUT_DIR/bl31.elf
else
	echo $ATF_FILE | grep -q -e "bl31.elf" || usage
	if [ ! -f $ATF_FILE ]; then
		echo $ATF_FILE: File not found!
		usage
	fi
	cp $ATF_FILE $OUTPUT_DIR/bl31.elf
fi

cp $HDF_FILE $BUILD_DIR/
cp $UBOOT_FILE $OUTPUT_DIR/u-boot.elf
cp $HDF_FILE $OUTPUT_DIR/

# get the tools version (e.g., v2018.3)
tool_version=$(hsi -version)
tool_version=${tool_version#hsi\ }
tool_version=${tool_version%\ (64-bit)*}

# Work-arownd for MPSoC ZCU102 and ZCU106 Evaluation Kits - DDR4 SODIMM change
# (https://www.xilinx.com/support/answers/71961.html)
if [ $tool_version == "v2018.3" ];then
(
	wget https://www.xilinx.com/Attachment/72113-files.zip -P $BUILD_DIR
	unzip $BUILD_DIR/72113-files.zip -d  $BUILD_DIR
)
fi

### Create create_fsbl_project.tcl file used by xsdk to create the fsbl
echo "hsi open_hw_design `basename $HDF_FILE`" > $BUILD_DIR/create_fsbl_project.tcl
echo 'set cpu_name [lindex [hsi get_cells -filter {IP_TYPE==PROCESSOR}] 0]' >> $BUILD_DIR/create_fsbl_project.tcl
echo 'sdk setws ./build/sdk' >> $BUILD_DIR/create_fsbl_project.tcl
echo "sdk createhw -name hw_0 -hwspec `basename $HDF_FILE`" >> $BUILD_DIR/create_fsbl_project.tcl
echo 'sdk createapp -name fsbl -hwproject hw_0 -proc $cpu_name -os standalone -lang C -app {Zynq MP FSBL}' >> $BUILD_DIR/create_fsbl_project.tcl
echo 'configapp -app fsbl build-config release' >> $BUILD_DIR/create_fsbl_project.tcl
if [ $tool_version == "v2018.3" ];then
(
	echo "file copy -force xfsbl_ddr_init.c ./build/sdk/fsbl/src" >> $BUILD_DIR/create_fsbl_project.tcl
	echo "file copy -force xfsbl_hooks.c    ./build/sdk/fsbl/src" >> $BUILD_DIR/create_fsbl_project.tcl
	echo "file copy -force xfsbl_hooks.h    ./build/sdk/fsbl/src" >> $BUILD_DIR/create_fsbl_project.tcl
)
fi
echo 'sdk projects -build -type all' >> $BUILD_DIR/create_fsbl_project.tcl

### Create create_pmufw_project.tcl
echo "set hwdsgn [open_hw_design `basename $HDF_FILE`]" > $BUILD_DIR/create_pmufw_project.tcl
echo 'generate_app -hw $hwdsgn -os standalone -proc psu_pmu_0 -app zynqmp_pmufw -sw pmufw -dir pmufw' >> $BUILD_DIR/create_pmufw_project.tcl
echo 'quit' >> $BUILD_DIR/create_pmufw_project.tcl

### Create zynq.bif file used by bootgen
echo "the_ROM_image:" > $OUTPUT_DIR/zynq.bif
echo "{" >> $OUTPUT_DIR/zynq.bif
echo "[bootloader,destination_cpu=a53-0] fsbl.elf" >> $OUTPUT_DIR/zynq.bif
echo "[pmufw_image] pmufw.elf" >> $OUTPUT_DIR/zynq.bif
echo "[destination_device=pl] system_top.bit" >> $OUTPUT_DIR/zynq.bif
echo "[destination_cpu=a53-0,exception_level=el-3,trustzone] bl31.elf" >> $OUTPUT_DIR/zynq.bif
echo "[destination_cpu=a53-0, exception_level=el-2] u-boot.elf" >> $OUTPUT_DIR/zynq.bif
echo "}" >> $OUTPUT_DIR/zynq.bif


### Build fsbl.elf & pmufw.elf
(
	cd $BUILD_DIR
	xsdk -batch -source create_fsbl_project.tcl
	hsi -source create_pmufw_project.tcl
	### There was a bug in some vivado version where they build would fail -> check CC_FLAGS
	grep "CC_FLAGS :=" pmufw/Makefile | grep -e "-Os" || sed -i '/-mxl-soft-mul/ s/$/ -Os -flto -ffat-lto-objects/' pmufw/Makefile
	cd pmufw
	make
)

### Copy fsbl and system_top.bit into the output folder
cp $BUILD_DIR/build/sdk/fsbl/Release/fsbl.elf $OUTPUT_DIR/fsbl.elf
cp $BUILD_DIR/build/sdk/hw_0/system_top.bit $OUTPUT_DIR/system_top.bit
cp $BUILD_DIR/pmufw/executable.elf $OUTPUT_DIR/pmufw.elf

### Build BOOT.BIN
(
	cd $OUTPUT_DIR
	bootgen -arch zynqmp -image zynq.bif -o BOOT.BIN -w
)

### Optionally tar.gz the entire output folder with the name given in argument 3
if [ ${#4} -ne 0 ]; then
	tar czvf $4.tar.gz $OUTPUT_DIR
fi
