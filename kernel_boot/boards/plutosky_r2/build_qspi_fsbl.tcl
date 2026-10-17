# SPDX-FileCopyrightText: 2026 Fu Wenbo
# SPDX-License-Identifier: AGPL-3.0-or-later

# Generate a Zynq FSBL matching the Pluto R2 Sky OpenWiFi XSA.
# Set OPENWIFI_FSBL_WORKSPACE and OPENWIFI_XSA before invoking XSCT.
if {![info exists ::env(OPENWIFI_FSBL_WORKSPACE)] ||
    ![info exists ::env(OPENWIFI_XSA)]} {
    error "OPENWIFI_FSBL_WORKSPACE and OPENWIFI_XSA must be set"
}

set workspace [file normalize $::env(OPENWIFI_FSBL_WORKSPACE)]
set xsa [file normalize $::env(OPENWIFI_XSA)]

setws $workspace
app create -name qspi_fsbl -hw $xsa -proc ps7_cortexa9_0 -os standalone \
    -template "Zynq FSBL"
app build -name qspi_fsbl
