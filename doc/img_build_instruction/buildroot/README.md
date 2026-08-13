# OpenWiFi Buildroot Images

[中文说明](README_CN.md)

## 1. Goal and current status

This port provides small, reproducible, fast-booting Buildroot SD-card images for OpenWiFi Zynq-7000 + AD9361 platforms. It replaces the multi-gigabyte general-purpose Linux image previously required for deployment.

The following targets have been booted and tested with basic OpenWiFi operation on real hardware:

| Build name | Hardware | Serial console | RAM |
| --- | --- | --- | --- |
| `antsdr_e200` | ANTSDR-E200 | `ttyPS0` | 512 MiB |
| `antsdr` | ANTSDR-E310/ANT | `ttyPS0` | 1 GiB |
| `e310v2` | ANTSDR-E310V2 | `ttyPS0` | 1 GiB |

All targets share one Linux kernel, module set and ext4 root filesystem. Hardware-specific SPL/U-Boot, device tree, FPGA bitstream, PS initialization and UART selection remain in each board's BOOT artifacts.

This is an additional build path. The existing ADI Kuiper large SD-card image
workflow and the OpenWrt workflow remain available in parallel and use their
original scripts, documentation and output locations. Buildroot does not call
or replace `user_space/update_sdcard.sh`, `user_space/prepare_kernel.sh`,
`user_space/boot_bin_gen.sh` or `kernel_boot/build_boot_bin.sh`.

The default SD layout is:

```text
SD card
├── partition 1: 64 MiB FAT32, label BOOT
└── partition 2: 96 MiB ext4, label rootfs
```

The complete SD image is about 169 MB. A compressed complete-system update package is typically about 20 MB, depending on the current package contents.

Main version baseline:

| Component | Version/source |
| --- | --- |
| Buildroot | `2025.02.11` submodule |
| Linux | Analog Devices Linux 6.12, commit `40201abd70d8f7848547a09fc7e366fba064483c` |
| U-Boot | Xilinx `xlnx_rebase_v2024.01_2024.2` |
| libad9361-iio | `v0.3` |
| OpenWiFi | Current repository source and short Git revision at build time |

## 2. Summary of implemented changes

### 2.1 Buildroot integration and repository ownership

- Buildroot `2025.02.11` is pinned as the `buildroot/` Git submodule.
- The submodule is kept unmodified.
- All OpenWiFi board definitions, packages, patches, overlays and image logic live in `buildroot-external/`.
- `buildroot-build.sh` is the single entry point for the shared build and board assembly.
- A curl-backed Buildroot downloader and GNU mirror fallback improve source-download reliability; Buildroot hashes still verify all archives.
- Generated `output/` files and the shared `dl/` cache are ignored by Git.

### 2.2 Shared system and board-specific boot artifacts

- `output/common/` builds the ARMv7 hard-float toolchain, Linux 6.12, OpenWiFi modules and rootfs once.
- The common kernel builds all three Linux DTBs.
- `output/<board>/` contains only board BOOT files, the final SD image and the complete firmware package.
- The matching `system_top.xsa` supplies both the FPGA bitstream and `ps7_init_gpl.c/h`.
- U-Boot DTS inputs and PS7 patches are generated without editing the DTS files tracked by OpenWiFi.

### 2.3 Boot chain

The BOOT partition uses familiar upstream OpenWiFi names:

```text
BOOT.BIN
u-boot.img
uEnv.txt
uImage
devicetree.dtb
system_top.bit.bin
```

Here `BOOT.BIN` is U-Boot SPL, rather than the Xilinx-FSBL composite used by the upstream Kuiper image. It therefore requires the separate `u-boot.img`; files from the two boot schemes must not be substituted by name alone.

The boot flow is:

```text
BootROM
  -> BOOT.BIN (U-Boot SPL + board PS7 initialization)
  -> u-boot.img
  -> import uEnv.txt
  -> configure the FPGA from system_top.bit.bin
  -> load uImage and devicetree.dtb
  -> mount /dev/mmcblk0p2 as the ext4 rootfs
```

The port updates old DTS tags to U-Boot 2024 `bootph-all`, validates the SPL UART/MMC/timer/stdout nodes during image assembly, fixes the actual UART path of every board, disables single-UART `DEBUG_LL` in the shared kernel, adds early SPL diagnostics for E200, and configures the FPGA before Linux starts.

### 2.4 Linux, AD9361 and OpenWiFi

- The build uses Analog Devices Linux 6.12 plus the existing OpenWiFi AD9361/AXI patches.
- The kernel has an `-openwifi` suffix and omits unused multimedia and debug stacks.
- `cfg80211`, `mac80211` and Xilinx DMA are loaded as modules after the SD rootfs is available.
- An AD9361 debugfs array-size mismatch in the selected patch set is fixed.
- OpenWiFi modules, side-channel modules, scripts, `sdrctl`, `side_ch_ctl`, `inject_80211` and `analyze_80211` are installed under `/root/openwifi`.
- Upstream user scripts are adapted to the minimal system: existing AD9361/Xilinx DMA modules are reused through `modprobe`, unnecessary `sudo` calls are removed, Debian `service` calls use Buildroot init scripts, and BusyBox `httpd` replaces the unavailable `webfsd`.
- `wgd.sh` verifies the AD9361/DDS/ADC chain, mounts debugfs and ensures the OpenWiFi interface is named `sdr0`.
- Verbose mac80211 HT/BA debug printk paths are disabled while debugfs remains available.

### 2.5 Runtime FPGA Manager support

U-Boot normally loads the FPGA. Reload drivers without touching the live FPGA/IIO chain with:

```sh
cd /root/openwifi
OPENWIFI_RELOAD_FPGA=0 ./wgd.sh 0
```

For development with a topology-compatible bitstream:

```sh
cd /root/openwifi
OPENWIFI_RELOAD_FPGA=1 ./wgd.sh 0
```

The reload path stops `iiod`, unbinds AD9361 and its DDS/ADC consumers, programs FPGA Manager, actively rebinds the RF chain and restarts `iiod` when required. Xilinx VDMA remains bound on E200 because its remove/probe path may access invalid PL registers during a full reload. All sysfs waits have a finite timeout controlled by `OPENWIFI_FPGA_WAIT_RETRIES` (50 attempts by default).

The automatic policy is scoped by system type: Buildroot reuses the FPGA
configured by U-Boot when the RF chain is ready, while Kuiper and OpenWrt keep
the historical default of reloading a local FPGA image. Explicit
`OPENWIFI_RELOAD_FPGA=0/1` always takes precedence. The `iiod` stop/restart
helper supports both Buildroot SysV init and Kuiper systemd services.

### 2.6 Root filesystem

The common musl rootfs includes OpenWiFi, hostapd, wpa_supplicant, ISC DHCP, wireless and IP tools, iperf3, tcpdump, libiio, iiod, IIO utilities, the newly packaged libad9361-iio and Dropbear. `/etc/motd` displays the OpenWiFi word mark, official repository and project description.

The board name is detected from `/proc/device-tree/model` at boot and written to `/etc/openwifi-board`, so it is not hard-coded into the shared rootfs.
BOOT is normally mounted read-only, `/tmp` and `/run` use tmpfs, and the Dropbear host identity created on first boot is persisted on BOOT across reboots and complete-system updates.

Shell programs in the OpenWiFi application notes that control drivers, interfaces, IIO or the FPGA run on the board. Plotting, waveform display, offline analysis and applications explicitly requiring a MATLAB/Python GUI continue to run on the host. This port supplies the embedded runtime; it does not place every host visualization dependency in the rootfs.

Defaults are:

```text
root password: openwifi
eth0:          192.168.10.122/24
serial:        115200 8N1
OpenWiFi dir:  /root/openwifi
```

### 2.7 Complete-system network update

Each board build creates a `.frm` containing all BOOT files and the complete compressed ext4 rootfs. It uses normal SSH/SCP, with no update daemon, dedicated port or token.

The updater validates board identity, sizes and SHA-256 digests, validates the uncompressed ext4 image, stages BOOT replacements, preserves Dropbear host keys, remounts the active rootfs read-only, writes `/dev/mmcblk0p2`, and reboots from tools staged in tmpfs. It uses `gzip -dc` and plain `tar` separately for BusyBox builds without `tar -z`. The host tool detaches after the completion marker so it does not hang on a stale SSH connection after reboot.

This is a single-rootfs design without automatic A/B rollback. Power must remain stable during the write.

## 3. Prerequisites

Install standard Buildroot host dependencies on Ubuntu or Debian:

```sh
sudo apt install build-essential git rsync cpio unzip bc file wget curl python3 \
    libncurses-dev
```

Initialize Buildroot after cloning OpenWiFi:

```sh
git submodule update --init buildroot
```

The default matching hardware input is:

```text
../openwifi-hw-img/boards/<board>/sdk/system_top.xsa
```

Override the hardware repository or a single XSA with:

```sh
OPENWIFI_HW_IMG_DIR=/path/to/openwifi-hw-img \
    ./buildroot-build.sh antsdr_e200 build

OPENWIFI_XSA=/path/to/system_top.xsa \
    ./buildroot-build.sh antsdr_e200 build
```

The XSA must match the selected board and FPGA design.

## 4. Build

Run from the OpenWiFi repository root:

```sh
./buildroot-build.sh antsdr_e200 build
./buildroot-build.sh antsdr build
./buildroot-build.sh e310v2 build
```

The first command builds the common system. Later board builds reuse the same kernel and rootfs and rebuild only the selected board's U-Boot and final artifacts.

Outputs include:

```text
output/common/images/uImage
output/common/images/rootfs.ext4
output/antsdr_e200/images/openwifi-antsdr_e200-sdcard.img
output/antsdr_e200/images/openwifi-antsdr_e200-system.frm
output/antsdr/images/openwifi-antsdr-sdcard.img
output/antsdr/images/openwifi-antsdr-system.frm
output/e310v2/images/openwifi-e310v2-sdcard.img
output/e310v2/images/openwifi-e310v2-system.frm
```

Additional commands:

```sh
./buildroot-build.sh antsdr_e200 configure
./buildroot-build.sh antsdr_e200 menuconfig
./buildroot-build.sh antsdr_e200 rebuild-system
./buildroot-build.sh antsdr_e200 clean
```

`rebuild-system` retains the toolchain but forces Linux, OpenWiFi, libad9361-iio and the rootfs to rebuild. `clean` removes only the selected board output and retains the common system and download cache.

## 5. Write an SD card

The first installation requires the complete image, not a manual copy of BOOT files:

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINTS
sudo umount /dev/sdX1 /dev/sdX2 2>/dev/null || true
sudo dd if=output/antsdr_e200/images/openwifi-antsdr_e200-sdcard.img \
    of=/dev/sdX bs=4M conv=fsync status=progress
sync
```

Replace `/dev/sdX` with the verified whole SD-card device, not a partition. This overwrites the selected device.

## 6. Boot and operate OpenWiFi

Connect the serial console at 115200 8N1 and log in as `root` with password `openwifi`.

```sh
cat /etc/openwifi-board
cat /proc/cmdline
iio_info -s
openwifi-start 0
ip link show sdr0
iw dev
```

`openwifi-start 0` is equivalent to running `./wgd.sh 0` in `/root/openwifi`. The currently verified stable default is `test_mode=0`; use other test modes only with a matching, explicitly tested FPGA/driver combination.

## 7. Update over Ethernet

Query, validate and install a complete E200 system package from the host:

```sh
./host-tools/openwifi_fw_update.py --host 192.168.10.122 status

./host-tools/openwifi_fw_update.py --host 192.168.10.122 check \
    output/antsdr_e200/images/openwifi-antsdr_e200-system.frm

./host-tools/openwifi_fw_update.py --host 192.168.10.122 update --reboot \
    output/antsdr_e200/images/openwifi-antsdr_e200-system.frm
```

The default password is `openwifi`. A package for a different board is rejected. Since the active rootfs is replaced in place, installation always requires `--reboot`.

Manual installation is also supported:

```sh
scp output/antsdr_e200/images/openwifi-antsdr_e200-system.frm \
    root@192.168.10.122:/tmp/openwifi-system.frm
ssh root@192.168.10.122
openwifi-fw-update status
openwifi-fw-update check /tmp/openwifi-system.frm
openwifi-fw-update install --reboot /tmp/openwifi-system.frm
```

## 8. Validation and limitations

- E200, E310/ANT and E310V2 have all been verified by the user to boot and run OpenWiFi on real hardware.
- The three targets share the same `uImage`, modules and `rootfs.ext4`, with board-specific BOOT artifacts.
- E200 AP/client association and iperf3 traffic have been exercised.
- The existing Kuiper build scripts and documentation are unchanged from
  upstream. The complete legacy SD-card preparation path was also exercised
  with the 2025-03-18 ADI Kuiper image: the image was flashed and byte-checked,
  both 32-bit and 64-bit Linux 6.12 kernels/modules were built, E200
  FSBL/BOOT.BIN/device-tree/bitstream artifacts were generated, both OpenWiFi
  driver architectures were installed, and the final FAT/ext4 checks passed.
- Two boards on the same management Ethernet cannot both retain `192.168.10.122`; change one management address temporarily.
- Prefer the FPGA loaded by U-Boot (`OPENWIFI_RELOAD_FPGA=0`) for normal operation. Runtime reload is a development function for compatible designs.
- A complete update overwrites the single active rootfs and has no automatic power-loss rollback.

Before committing, verify that no generated change entered the Buildroot submodule:

```sh
git -C buildroot status --short
git submodule status buildroot
```

## 9. File-level change index

| Path | Purpose |
| --- | --- |
| `.gitmodules`, `.gitignore`, `.gitattributes` | Register Buildroot, ignore outputs/caches and define patch whitespace handling |
| `README.md` | English documentation linked by default from the main project README |
| `README_CN.md` | Optional Chinese translation of the Buildroot documentation |
| `buildroot-build.sh` | Validate board/XSA inputs, maintain the common output and assemble board images |
| `buildroot-external/Config.in`, `external.desc`, `external.mk` | Define the external tree, board choices, packages, DTB installation and console fixes |
| `buildroot-external/configs/openwifi_common_defconfig` | Configure the shared ARM/Linux/rootfs/U-Boot system and target packages |
| `buildroot-external/board/common/linux-*.fragment` | Minimize Linux and configure DMA, mac80211 logging and safe reboot support |
| `buildroot-external/board/common/uboot.fragment` | Configure SPL/U-Boot, FPGA support and the `uEnv.txt` boot command |
| `buildroot-external/board/common/genimage.cfg`, `system_top.bif.in`, `uEnv.txt.in` | Define SD partitions, bitstream conversion and the BOOT environment |
| `buildroot-external/board/common/rootfs-overlay/` | Provide fstab, Ethernet, MOTD, DHCP compatibility, BOOT bitstream links and persistent SSH identity |
| `buildroot-external/board/common/openwifi-start`, `openwifi-board-name`, `S02openwifi-board` | Provide the OpenWiFi entry point and runtime board detection |
| `buildroot-external/board/common/openwifi-fw-update` | Validate, install and reboot after a complete system package |
| `buildroot-external/board/common/curl-wget`, `busybox-openwifi.fragment`, `post-build.sh` | Improve downloads, enable BusyBox features and remove retired updater files |
| `buildroot-external/package/openwifi/` | Build/install OpenWiFi modules, tools and Buildroot-adapted scripts |
| `buildroot-external/package/libad9361-iio/` | Package libad9361-iio with its version hash and CMake options |
| `buildroot-external/patches/linux/` | Fix the AD9361 debugfs entry capacity |
| `buildroot-external/patches/uboot/antsdr_e200/` | Add E200 SPL MMC/reset diagnostics |
| `buildroot-external/support/prepare-board.py` | Generate PS7 patches from XSA, fix U-Boot DTS and create board `uEnv.txt` |
| `buildroot-external/support/fix-linux-dts-console.py` | Set the correct Linux `stdout-path` for each board |
| `buildroot-external/support/assemble-board.sh` | Validate SPL and generate bitstream, BOOT FAT, SD image and board outputs |
| `buildroot-external/support/mk-firmware-package.sh` | Create complete `.frm` files with manifest, sizes and SHA-256 digests |
| `host-tools/openwifi_fw_update.py` | Validate/update over SSH/SCP and detach reliably after board reboot |
| `user_space/wgd.sh` | Add FPGA reload policy, RF-chain checks, debugfs setup and stable `sdr0` naming |
| `user_space/load_fpga_img.sh` | Safely reload FPGA, rebind AD9361/IIO and enforce finite waits |
