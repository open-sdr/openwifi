#!/bin/sh
set -eu

[ "$#" -eq 4 ] || {
	echo "usage: $0 <board> <boot-files-dir> <output.frm> <version>" >&2
	exit 2
}

BOARD=$1
BOOT_FILES_DIR=$2
OUTPUT=$3
VERSION=$4
WORK_DIR=$(mktemp -d)
trap 'rm -rf "${WORK_DIR}"' EXIT HUP INT TERM

case "${BOARD}" in
	antsdr_e200|antsdr|e310v2) ;;
	*) echo "Unsupported board: ${BOARD}" >&2; exit 1 ;;
esac

mkdir -p "${WORK_DIR}/files"
for file in BOOT.BIN u-boot.img uEnv.txt uImage devicetree.dtb \
	system_top.bit.bin; do
	[ -s "${BOOT_FILES_DIR}/${file}" ] || {
		echo "Missing firmware input: ${BOOT_FILES_DIR}/${file}" >&2
		exit 1
	}
	cp -L "${BOOT_FILES_DIR}/${file}" "${WORK_DIR}/files/${file}"
done
[ -s "${BOOT_FILES_DIR}/rootfs.ext4" ] || {
	echo "Missing firmware input: ${BOOT_FILES_DIR}/rootfs.ext4" >&2
	exit 1
}
gzip -c "${BOOT_FILES_DIR}/rootfs.ext4" > \
	"${WORK_DIR}/files/rootfs.ext4.gz"

{
	echo "format=openwifi-zynq-system-v1"
	echo "board=${BOARD}"
	echo "mode=system"
	echo "version=${VERSION}"
	echo "rootfs.uncompressed_size=$(stat -c %s "${BOOT_FILES_DIR}/rootfs.ext4")"
	echo "rootfs.uncompressed_sha256=$(sha256sum "${BOOT_FILES_DIR}/rootfs.ext4" | awk '{print $1}')"
	echo "created_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
	for file in BOOT.BIN u-boot.img uEnv.txt uImage devicetree.dtb \
		system_top.bit.bin rootfs.ext4.gz; do
		echo "file.${file}.size=$(stat -c %s "${WORK_DIR}/files/${file}")"
		echo "file.${file}.sha256=$(sha256sum "${WORK_DIR}/files/${file}" | awk '{print $1}')"
	done
} > "${WORK_DIR}/manifest"

mkdir -p "$(dirname "${OUTPUT}")"
tar -C "${WORK_DIR}" -czf "${OUTPUT}" manifest files
echo "OpenWiFi ${BOARD} complete SD firmware: ${OUTPUT}"
