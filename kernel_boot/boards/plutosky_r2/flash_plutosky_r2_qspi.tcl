# SPDX-FileCopyrightText: 2026 Fu Wenbo
# SPDX-License-Identifier: AGPL-3.0-or-later

# Run with XSCT after connecting the board's JTAG/debug interface.
# The release directory must also contain plutosky_r2_qspi_BOOT.BIN and
# qspi_fsbl.elf. This writes the QSPI range starting at offset zero.
if {![info exists ::env(OPENWIFI_FLASH_ACK)] ||
    $::env(OPENWIFI_FLASH_ACK) ne "I_UNDERSTAND"} {
    error "Set OPENWIFI_FLASH_ACK=I_UNDERSTAND before programming QSPI"
}

set release_dir [file dirname [file normalize [info script]]]
set image [file join $release_dir plutosky_r2_qspi_BOOT.BIN]
set fsbl [file join $release_dir qspi_fsbl.elf]
foreach file [list $image $fsbl] {
    if {![file isfile $file]} {
        error "Required file is missing: $file"
    }
}

exec program_flash -f $image -offset 0 -flash_type qspi-x4-single -fsbl $fsbl \
    -blank_check -verify -cable type xilinx_tcf url tcp:localhost:3121
