#!/bin/bash
set -ex

XSA_FILE=$1
UBOOT_FILE=$2
ATF_FILE=${3:-download}
BUILD_DIR=build_boot_bin
OUTPUT_DIR=output_boot_bin

usage () {
	echo "usage: $0 filename.xsa u-boot.elf  (download | bl31.elf | <path-to-arm-trusted-firmware-source>) [output-archive]"
	exit 1
}

depends () {
	echo "Xilinx $1 must be installed and in your PATH"
	echo "try: source /opt/Xilinx/Vivado/202x.x/settings64.sh"
	exit 1
}

### Check command line parameters
echo $XSA_FILE | grep -q ".xsa" || usage
echo $UBOOT_FILE | grep -q -e ".elf" -e "uboot" -e "u-boot" || usage

if [ ! -f $XSA_FILE ]; then
	echo $XSA_FILE: File not found!
	usage
fi

if [ ! -f $UBOOT_FILE ]; then
    echo $UBOOT_FILE: File not found!
    usage
fi

### Check for required Xilinx tools (starting with 2019.2 there is no hsi anymore)
command -v xsct >/dev/null 2>&1 || depends xsct
command -v bootgen >/dev/null 2>&1 || depends bootgen

rm -Rf $BUILD_DIR $OUTPUT_DIR
mkdir -p $OUTPUT_DIR
mkdir -p $BUILD_DIR

# 2020.1 use bf72e4d494f3be10665b94c0e88766eb2096ef71
# 2021.2 use 799131a3b063f6f24f87baa74e46906c076aebcd
# 2022.2 use 5ebf70ea38e4626637568352b644acbffe3b13c1
# 2023.1 use c7385e021c0b95a025f2c78384d57224e0120401
# 2023.2 use 04013814718e870261f27256216cd7da3eda6a5d

tool_version=$(vitis -v | grep -o "Vitis v20[1-9][0-9]\.[0-9] (64-bit)" | grep -o "v20[1-9][0-9]\.[0-9]")
if [[ "$tool_version" != "v20"[1-9][0-9]"."[0-9] ]] ; then
	echo "Could not determine Vitis version"
	exit 1
fi
atf_version=xilinx-$tool_version

if [[ "$atf_version" == "xilinx-v2021.1" ]];then atf_version="xlnx_rebase_v2.4_2021.1";fi
if [[ "$atf_version" == "xilinx-v2021.1.1" ]];then atf_version="xlnx_rebase_v2.4_2021.1_update1";fi
if [[ "$atf_version" == "xilinx-v2021.2" ]];then atf_version="xilinx-v2021.2";fi
if [[ "$atf_version" == "xilinx-v2022.2" ]];then atf_version="xilinx-v2022.2";fi
if [[ "$atf_version" == "xilinx-v2023.1" ]];then atf_version="xilinx-v2023.1";fi
if [[ "$atf_version" == "xilinx-v2023.2" ]];then atf_version="xilinx-v2023.2";fi

if [[ "$4" == "uart1" ]];then console="cadence1";else console="cadence0";fi

### Check if ATF_FILE is .elf or path to arm-trusted-firmware
if [ "$ATF_FILE" != "" ] && [ -d $ATF_FILE ]; then
### Build arm-trusted-firmware bl31.elf
(
	cd $ATF_FILE
	make distclean
	git checkout $atf_version
	make CROSS_COMPILE=aarch64-linux-gnu- PLAT=zynqmp RESET_TO_BL31=1 ZYNQMP_CONSOLE=$console
)
	cp $ATF_FILE/build/zynqmp/release/bl31/bl31.elf $OUTPUT_DIR/bl31.elf
elif [ "$ATF_FILE" == "download" ]; then
(
	command -v git >/dev/null 2>&1 || depends git
	cd $BUILD_DIR
	git clone https://github.com/Xilinx/arm-trusted-firmware.git
	cd arm-trusted-firmware
	git checkout $atf_version
	make CROSS_COMPILE=aarch64-linux-gnu- PLAT=zynqmp RESET_TO_BL31=1 ZYNQMP_CONSOLE=$console
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

cp "$XSA_FILE" "$BUILD_DIR/"
cp "$UBOOT_FILE" "$OUTPUT_DIR/u-boot.elf"
cp "$XSA_FILE" "$OUTPUT_DIR/"

### Get basename of xsa and bit file
XSA_FILE_BASENAME="$(basename $XSA_FILE)"
XSA_FILE_BASENAME_WO_EXT="$(basename $XSA_FILE .xsa)"
BIT_FILE_BASENAME="$XSA_FILE_BASENAME_WO_EXT.bit"

### Create create_fsbl_project.tcl file used by xsct to create the fsbl.
echo "hsi open_hw_design $XSA_FILE_BASENAME" > $BUILD_DIR/create_fsbl_project.tcl
echo 'set cpu_name [lindex [hsi get_cells -filter {IP_TYPE==PROCESSOR}] 0]' >> $BUILD_DIR/create_fsbl_project.tcl
echo "platform create -name hw0 -hw $XSA_FILE_BASENAME -os standalone -out ./build/sdk -proc \$cpu_name" >> $BUILD_DIR/create_fsbl_project.tcl
echo 'platform generate' >> $BUILD_DIR/create_fsbl_project.tcl

FSBL_PATH="$BUILD_DIR/build/sdk/hw0/export/hw0/sw/hw0/boot/fsbl.elf"
SYSTEM_TOP_BIT_PATH="$BUILD_DIR/build/sdk/hw0/hw/$BIT_FILE_BASENAME"
PMUFW_PATH="$BUILD_DIR/build/sdk/hw0/export/hw0/sw/hw0/boot/pmufw.elf"

### Create zynq.bif file used by bootgen
echo "the_ROM_image:" > $OUTPUT_DIR/zynq.bif
echo "{" >> $OUTPUT_DIR/zynq.bif
echo "[bootloader,destination_cpu=a53-0] fsbl.elf" >> $OUTPUT_DIR/zynq.bif
echo "[pmufw_image] pmufw.elf" >> $OUTPUT_DIR/zynq.bif
echo "[destination_device=pl] $BIT_FILE_BASENAME" >> $OUTPUT_DIR/zynq.bif
echo "[destination_cpu=a53-0,exception_level=el-3,trustzone] bl31.elf" >> $OUTPUT_DIR/zynq.bif
echo "[destination_cpu=a53-0, exception_level=el-2] u-boot.elf" >> $OUTPUT_DIR/zynq.bif
echo "}" >> $OUTPUT_DIR/zynq.bif

### Build fsbl.elf & pmufw.elf
(
	cd $BUILD_DIR
	xsct create_fsbl_project.tcl
)
### Copy fsbl and $BIT_FILE_BASENAME into the output folder
cp "$FSBL_PATH" "$OUTPUT_DIR/fsbl.elf"
cp "$SYSTEM_TOP_BIT_PATH" "$OUTPUT_DIR/$BIT_FILE_BASENAME"
cp "$PMUFW_PATH" "$OUTPUT_DIR/pmufw.elf"

### Build BOOT.BIN
(
	cd $OUTPUT_DIR
	bootgen -arch zynqmp -image zynq.bif -o BOOT.BIN -w
)

### Optionally tar.gz the entire output folder with the name given in argument 4/5
if [[ ( $4 == "uart"* && ${#5} -ne 0 ) ]]; then
	tar czvf $5.tar.gz $OUTPUT_DIR
fi

if [[ ( ${#4} -ne 0 && $4 != "uart"* && ${#5} -eq 0 ) ]]; then
        tar czvf $4.tar.gz $OUTPUT_DIR
fi
