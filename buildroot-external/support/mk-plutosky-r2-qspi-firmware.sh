#!/bin/sh
# SPDX-FileCopyrightText: 2026 Fu Wenbo
# SPDX-License-Identifier: AGPL-3.0-or-later
set -eu

[ "$#" -eq 2 ] || {
	echo "usage: $0 <image-dir> <output.dfu>" >&2
	exit 2
}

IMAGE_DIR=$1
OUTPUT=$2
MAX_PAYLOAD_BYTES=$((0x1e00000))
MKIMAGE=${MKIMAGE:-mkimage}
MKIMAGE_DTC_COMMAND=${MKIMAGE_DTC_COMMAND:-}
WORK_DIR=$(mktemp -d)
trap 'rm -rf "${WORK_DIR}"' EXIT HUP INT TERM

for file in zImage devicetree.dtb system_top.bit rootfs.cpio.gz; do
	[ -s "${IMAGE_DIR}/${file}" ] || {
		echo "Missing QSPI firmware input: ${IMAGE_DIR}/${file}" >&2
		exit 1
	}
done

ITS="${WORK_DIR}/plutosky-r2-openwifi.its"
FIT="${WORK_DIR}/plutosky-r2-openwifi.itb"

cat >"${ITS}" <<EOF
/dts-v1/;

/ {
	description = "OpenWiFi Pluto R2 Sky QSPI firmware";
	#address-cells = <1>;

	images {
		fdt@1 {
			description = "Pluto R2 Sky device tree";
			data = /incbin/("${IMAGE_DIR}/devicetree.dtb");
			type = "flat_dt";
			arch = "arm";
			compression = "none";
		};

		fpga@1 {
			description = "OpenWiFi FPGA bitstream";
			data = /incbin/("${IMAGE_DIR}/system_top.bit");
			type = "fpga";
			arch = "arm";
			compression = "none";
			load = <0x0f000000>;
		};

		linux_kernel@1 {
			description = "OpenWiFi Linux kernel";
			data = /incbin/("${IMAGE_DIR}/zImage");
			type = "kernel";
			arch = "arm";
			os = "linux";
			compression = "none";
			load = <0x00008000>;
			entry = <0x00008000>;
		};

		ramdisk@1 {
			description = "OpenWiFi Buildroot ramdisk";
			data = /incbin/("${IMAGE_DIR}/rootfs.cpio.gz");
			type = "ramdisk";
			arch = "arm";
			os = "linux";
			compression = "gzip";
		};
	};

	configurations {
		default = "config@0";
EOF

for config in $(seq 0 10); do
	cat >>"${ITS}" <<EOF
		config@${config} {
			description = "OpenWiFi Pluto R2 Sky";
			fdt = "fdt@1";
			kernel = "linux_kernel@1";
			ramdisk = "ramdisk@1";
			fpga = "fpga@1";
		};
EOF
done

cat >>"${ITS}" <<'EOF'
	};
};
EOF

if [ -n "${MKIMAGE_DTC_COMMAND}" ]; then
	# mkimage shells out to dtc while assembling the FIT. Put the caller's
	# dtc first on PATH: -D takes a list of dtc options, not a dtc path.
	PATH="$(dirname "${MKIMAGE_DTC_COMMAND}"):${PATH}"
	export PATH
fi

"${MKIMAGE}" -f "${ITS}" "${FIT}"

FIT_SIZE=$(stat -c %s "${FIT}")
[ "${FIT_SIZE}" -le "${MAX_PAYLOAD_BYTES}" ] || {
	echo "QSPI firmware is ${FIT_SIZE} bytes; maximum is ${MAX_PAYLOAD_BYTES}" >&2
	exit 1
}

mkdir -p "$(dirname "${OUTPUT}")"
python3 - "${FIT}" "${OUTPUT}" <<'PY'
import binascii
import pathlib
import struct
import sys

fit = pathlib.Path(sys.argv[1]).read_bytes()
# The original Pluto updater accepts a normal FIT image with this standard
# DFU suffix. Its firmware alternate writes it at QSPI offset 0x00200000.
suffix = struct.pack("<HHHH3sB", 0xFFFF, 0xB673, 0x0456, 0x0100, b"UFD", 16)
payload = fit + suffix
crc = binascii.crc32(payload) & 0xFFFFFFFF
pathlib.Path(sys.argv[2]).write_bytes(payload + struct.pack("<I", crc))
PY

echo "OpenWiFi Pluto R2 Sky QSPI firmware: ${OUTPUT}"
