#!/bin/bash

# Author: Robbe Gaeremynck

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
SCRIPT_PATH="$SCRIPT_DIR/$(basename -- "${BASH_SOURCE[0]}")"

if ! cd "$SCRIPT_DIR"; then
  echo "ERROR: Could not change directory to $SCRIPT_DIR"
  exit 1
fi

BOARD_NAME=$1
ARCH=$2
DEFAULT_DTS_FOLDER="${3:-./defaults}"
DTSI_FOLDER="${4:-$DEFAULT_DTS_FOLDER/include}"

usage () {
	echo "usage: $0 \$BOARD_NAME \$ARCH (32 or 64) optional: custom dts folder optional: custom dtsi folder"
	exit 1
}

set -x

# Arguments short
if [ "$#" -lt 2 ]; then
    usage
    exit 1
fi

OPENWIFI_OVERLAY_FILE_SOURCE="openwifi_${ARCH}_ad9361.dtso"

echo "---Compiling the device tree overlays---"
echo "---Compiling openwifi overlay: $OPENWIFI_OVERLAY_FILE_SOURCE---"
dtc -@ -I dts -O dtb -o openwifi.dtbo $OPENWIFI_OVERLAY_FILE_SOURCE
echo "---Compiling openwifi $BOARD_NAME overlay---"
dtc -@ -I dts -O dtb -o ./$BOARD_NAME/$BOARD_NAME.dtbo ./overlays/$BOARD_NAME.dtso

# Check if fixed devicetree.dts present (if so, we should only compile overlays)
if [ -f "$BOARD_NAME/devicetree.dts" ]; then
  echo "WARNING: There is a fixed device tree present for $BOARD_NAME, only overlays are compiled"
  exit 0
fi

echo "---Compiling default $BOARD_NAME device tree---"

declare -A openwifi_name_to_kernel_dts
openwifi_name_to_kernel_dts=(
  ["adrv9361z7035"]="zynq-adrv9361.dts"
  ["adrv9364z7020"]="zynq-adrv9364.dts"
  ["antsdr_e200"]="zynq-antsdre200.dts"
  ["antsdr"]="zynq-antsdre310.dts"
  ["e310v2"]="zynq-antsdre310v2.dts"
  ["zed_fmcs2"]="zynq-zed.dts"
  ["zc702_fmcs2"]="zynq-zc702.dts"
  ["zc706_fmcs2"]="zynq-zc706.dts"
  ["zcu102_fmcs2"]="zynqmp-zcu102-rev1.1.dts"
)
DEFAULT_DTS_FILENAME=${openwifi_name_to_kernel_dts[$BOARD_NAME]}

# Check if DTS exists in DTS folder
if [ ! -f "$DEFAULT_DTS_FOLDER/$DEFAULT_DTS_FILENAME" ]; then
  if [ "$#" -gt 2 ]; then # If file not found and non-defaults used, call yourself again, but this time with defaults
    "$SCRIPT_PATH" "$BOARD_NAME" "$ARCH"
    exit 1
  fi
  echo "WARNING: No default device tree present, only compiling overlays"
  exit 0
fi

echo "---Generating the default (non-openwifi) device tree for $BOARD_NAME---"
cpp -nostdinc -x assembler-with-cpp -I$DTSI_FOLDER -o ./$BOARD_NAME/default_devicetree.dts $DEFAULT_DTS_FOLDER/$DEFAULT_DTS_FILENAME
dtc -@ -O dtb -o ./$BOARD_NAME/default_devicetree.dtb ./$BOARD_NAME/default_devicetree.dts

echo "---Applying overlays onto default device tree---"
fdtoverlay -i ./$BOARD_NAME/default_devicetree.dtb -o ./$BOARD_NAME/devicetree.dtb -v openwifi.dtbo ./$BOARD_NAME/$BOARD_NAME.dtbo
echo "---Decompiling the device tree (sanity check)---"
dtc -I dtb -O dts -o ./$BOARD_NAME/full_devicetree.dts ./$BOARD_NAME/devicetree.dtb