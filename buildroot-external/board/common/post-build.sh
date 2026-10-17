#!/bin/sh
set -eu

TARGET_DIR=$1

# Clean files installed by the retired experimental A/B updater and old
# bitstream-link init script when reusing an existing Buildroot output tree.
rm -f "${TARGET_DIR}/usr/sbin/openwifi-firmware-updated" \
	"${TARGET_DIR}/etc/init.d/S45openwifi-firmware-updated" \
	"${TARGET_DIR}/etc/openwifi-update.conf" \
	"${TARGET_DIR}/etc/init.d/S41openwifi-slot-files"

chmod 0755 "${TARGET_DIR}/etc/init.d/S42plutosky-r2-usb-gadget" \
	"${TARGET_DIR}/etc/init.d/S43plutosky-r2-usb-console"
