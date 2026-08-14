#!/bin/sh
set -eu

[ "$#" -eq 7 ] || {
	echo "usage: $0 <board> <common-output> <board-output> <xsa> <generated-dir> <external-dir> <repo-dir>" >&2
	exit 2
}

BOARD=$1
COMMON_OUTPUT=$2
BOARD_OUTPUT=$3
XSA=$4
GENERATED_DIR=$5
EXTERNAL_DIR=$6
REPO_DIR=$7
HOST_DIR="${COMMON_OUTPUT}/host"
COMMON_IMAGES="${COMMON_OUTPUT}/images"
BINARIES_DIR="${BOARD_OUTPUT}/images"
BUILD_DIR="${BOARD_OUTPUT}/build"
SPL_DTB="${COMMON_OUTPUT}/build/uboot-custom/spl/u-boot-spl.dtb"

case "${BOARD}" in
	antsdr_e200) bus=amba; uart=e0000000 ;;
	antsdr) bus=amba; uart=e0001000 ;;
	e310v2) bus=axi; uart=e0001000 ;;
	*) echo "Unsupported board: ${BOARD}" >&2; exit 1 ;;
esac

for file in "${XSA}" "${COMMON_IMAGES}/uImage" \
	"${COMMON_IMAGES}/rootfs.ext4" "${COMMON_IMAGES}/${BOARD}.dtb" \
	"${COMMON_IMAGES}/boot.bin" "${COMMON_IMAGES}/u-boot.img" \
	"${SPL_DTB}"; do
	[ -f "${file}" ] || { echo "Missing image input: ${file}" >&2; exit 1; }
done

# A silently pruned SPL tree produces no UART and cannot load u-boot.img.
for node in "/${bus}/serial@${uart}" "/${bus}/mmc@e0100000"; do
	[ "$("${HOST_DIR}/bin/fdtget" "${SPL_DTB}" "${node}" status)" = okay ] || {
		echo "Missing enabled SPL node: ${node}" >&2
		exit 1
	}
done
[ "$("${HOST_DIR}/bin/fdtget" "${SPL_DTB}" /chosen stdout-path)" = \
	"/${bus}/serial@${uart}:115200n8" ] || {
	echo "Invalid SPL stdout-path for ${BOARD}" >&2
	exit 1
}
[ "$("${HOST_DIR}/bin/fdtget" "${SPL_DTB}" "/${bus}/timer@f8f00600" compatible)" = \
	"arm,cortex-a9-twd-timer" ] || {
	echo "Missing SPL TWD timer" >&2
	exit 1
}

# This directory contains board-specific generated artifacts only. Recreate it
# so an old A/B layout or symlink cannot leak into a newly assembled image.
rm -rf "${BINARIES_DIR}"
mkdir -p "${BINARIES_DIR}" "${BUILD_DIR}"
cp -f "${COMMON_IMAGES}/boot.bin" "${BINARIES_DIR}/BOOT.BIN"
cp -f "${COMMON_IMAGES}/u-boot.img" "${BINARIES_DIR}/u-boot.img"
cp -f "${COMMON_IMAGES}/${BOARD}.dtb" "${BINARIES_DIR}/devicetree.dtb"
cp -Lf "${COMMON_IMAGES}/uImage" "${BINARIES_DIR}/uImage"
cp -Lf "${COMMON_IMAGES}/rootfs.ext4" "${BINARIES_DIR}/rootfs.ext4"
cp -f "${GENERATED_DIR}/uEnv.txt" "${BINARIES_DIR}/uEnv.txt"

unzip -p "${XSA}" system_top.bit > "${BINARIES_DIR}/system_top.bit"
sed 's|@BITFILE@|system_top.bit|' "${EXTERNAL_DIR}/board/common/system_top.bif.in" \
	> "${BINARIES_DIR}/system_top.bif"
(
	cd "${BINARIES_DIR}"
	"${HOST_DIR}/bin/bootgen" -arch zynq -image system_top.bif \
		-process_bitstream bin -w on
)

export BUILD_DIR HOST_DIR BINARIES_DIR
export TARGET_DIR="${COMMON_OUTPUT}/target"
export BR2_CONFIG="${COMMON_OUTPUT}/.config"
export BR2_EXTERNAL_OPENWIFI_PATH="${EXTERNAL_DIR}"
PATH="${HOST_DIR}/bin:${HOST_DIR}/sbin:${PATH}"
export PATH
"${REPO_DIR}/buildroot/support/scripts/genimage.sh" \
	-c "${EXTERNAL_DIR}/board/common/genimage.cfg"

BOOT_FILES_DIR="${BINARIES_DIR}/boot-files"
mkdir -p "${BOOT_FILES_DIR}"
for file in BOOT.BIN u-boot.img uEnv.txt uImage devicetree.dtb \
	system_top.bit.bin; do
	cp -f "${BINARIES_DIR}/${file}" "${BOOT_FILES_DIR}/${file}"
done
cp -Lf "${BINARIES_DIR}/rootfs.ext4" "${BOOT_FILES_DIR}/rootfs.ext4"

ln -snf sdcard.img "${BINARIES_DIR}/openwifi-${BOARD}-sdcard.img"

VERSION=$(git -C "${REPO_DIR}" describe --always --dirty 2>/dev/null || echo unknown)
"${EXTERNAL_DIR}/support/mk-firmware-package.sh" \
	"${BOARD}" "${BOOT_FILES_DIR}" \
	"${BINARIES_DIR}/openwifi-${BOARD}-system.frm" "${VERSION}"

echo "OpenWiFi ${BOARD} SD image: ${BINARIES_DIR}/openwifi-${BOARD}-sdcard.img"
