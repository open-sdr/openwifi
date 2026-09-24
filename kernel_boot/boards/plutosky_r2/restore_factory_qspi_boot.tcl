# SPDX-FileCopyrightText: 2026 Fu Wenbo
# SPDX-License-Identifier: AGPL-3.0-or-later

# Restore the vendor QSPI boot loader through XSCT/JTAG. Afterwards, use the
# vendor pluto.dfu update package to restore the complete factory firmware.
if {![info exists ::env(OPENWIFI_FLASH_ACK)] ||
    $::env(OPENWIFI_FLASH_ACK) ne "I_UNDERSTAND"} {
    error "Set OPENWIFI_FLASH_ACK=I_UNDERSTAND before programming QSPI"
}

set release_dir [file dirname [file normalize [info script]]]
set image [file join $release_dir factory_recovery boot.bin]
set fsbl [file join $release_dir factory_recovery fsbl.elf]
foreach file [list $image $fsbl] {
    if {![file isfile $file]} {
        error "Required file is missing: $file"
    }
}

exec program_flash -f $image -offset 0 -flash_type qspi-x4-single -fsbl $fsbl \
    -blank_check -verify -cable type xilinx_tcf url tcp:localhost:3121
