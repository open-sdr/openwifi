# AGENTS.md

Guidance for AI coding agents working in the openwifi repository. This file is
the single source of truth for all agents. Keep agent-specific files (such as
`CLAUDE.md`) thin and point them here.

## Project overview

openwifi is a Linux `mac80211`-compatible, full-stack IEEE 802.11 (Wi-Fi)
design built on Software Defined Radio (SDR). This repository holds the **Linux
driver and user-space software**; the FPGA design lives in the separate
`open-sdr/openwifi-hw` repository and prebuilt bitstreams in
`open-sdr/openwifi-hw-img`. Target hardware is Xilinx Zynq-7000 (32-bit) and
Zynq UltraScale+ MPSoC (64-bit) boards paired with an AD9361/AD9364 RF front
end. Licensing is dual: AGPLv3 for open source, commercial otherwise.

## Repository layout

- `driver/`: the kernel driver. `sdr.c` is the main `mac80211` driver,
  `sdrctl_intf.c` the netlink interface behind the `sdrctl` tool, and
  `hw_def.h` / `sysfs_intf.c` the register and sysfs glue. Subdirectories are
  per-block kernel modules: `openofdm_rx/`, `openofdm_tx/` (OFDM PHY),
  `rx_intf/`, `tx_intf/` (data-path interfaces), `side_ch/` (CSI/IQ side
  channel), `xpu/` (low-MAC timing and CSMA/CA control), and `xilinx_dma/`.
  `make_all.sh` builds every module.
- `user_space/`: tools and scripts for the board or host. C tools live in
  `sdrctl_src/` (`sdrctl`), `side_ch_ctl_src/` (`side_ch_ctl` plus Python
  capture scripts), `inject_80211/`, and `fast_reg_log/`. Shell helpers cover
  RF init, monitoring, and setup; `prepare_kernel.sh`, `boot_bin_gen.sh`, and
  `drv_and_fpga_package_gen.sh` drive the Kuiper build and packaging.
- `kernel_boot/`: kernel configs (`kernel_config`, `kernel_config_zynqmp`),
  AD9361 and HDMI patches, board boot files and device-tree overlays under
  `boards/`, and `build_boot_bin.sh` / `build_zynqmp_boot_bin.sh`.
- `adi-linux/`, `adi-linux-64/`: Analog Devices Linux kernel sources
  (submodules) used as the build tree for the 32-bit and 64-bit targets.
- `doc/`: documentation, including build instructions in
  `doc/img_build_instruction/` (`kuiper.md`, `openwrt/`), `app_notes/`, and
  `known_issue/`.
- `.github/`: CI workflows (`workflows/`) and GitHub Copilot skills (`skills/`).

## Building

Two independent build paths, both documented under
`doc/img_build_instruction/`:

- **Kuiper (Analog Devices SD-card image):** build the kernel with
  `user_space/prepare_kernel.sh`, then the driver modules with
  `driver/make_all.sh` (each takes the Xilinx tools directory and the target
  arch, `32` or `64`). See `doc/img_build_instruction/kuiper.md`.
- **OpenWrt:** build per-board images from `doc/img_build_instruction/openwrt/`
  with the Docker-based flow (`build_images.sh` for all boards,
  `build_image_for_board.sh` for one). No Vivado required. See
  `doc/img_build_instruction/openwrt/README.md`.

## Continuous integration

`.github/workflows/build-openwrt-images.yml` builds the OpenWrt images. It fans
each board out into its own job (to stay under the per-job time limit) and warms
one shared toolchain per target architecture through the Actions cache. A `gate`
job decides when to run (version tags `openwrt-openwifi_v*`, the
`openwrt-version` PR label, or changes under the OpenWrt build paths). Which
openwifi source the feed compiles is controlled by the `OPENWIFI_GITHUB_OWNER`
and `OPENWIFI_BRANCH` variables.

## Related repositories

- `open-sdr/openwifi-hw`: FPGA design (IP cores, per-board Vivado projects).
- `open-sdr/openwifi-hw-img`: prebuilt FPGA bitstreams (`system_top.xsa`).
- `open-sdr/openwrt-openwifi`: OpenWrt (buildroot) fork with openwifi configs.
- `open-sdr/openwrt-openwifi-packages-feed`: the OpenWrt package feed;
  `kernel/openwifi/Makefile` pins the openwifi source branch and revision.

## Git and versioning conventions

Use Conventional Commits, Conventional Branch naming, and Semantic Versioning.
Apply them whenever the user asks you to create a branch, write a commit, or
choose a release version. Do not commit, push, or open a pull request unless the
user asked you to; the formats below apply when they did.

The full specifications, with examples and validation, live in the
conditionally loaded skills under `.github/skills/` (read by GitHub Copilot):
`conventional-branch`, `conventional-commits`, and `semantic-versioning`.

### Branch names

Format `<type>/<short-kebab-description>`, all lowercase and hyphen-separated.
Types: `feature` (or `feat`), `bugfix` (or `fix`), `hotfix`, `release`, `chore`,
plus the AI-source prefixes `ai`, `copilot`, `cursor`, `claude`, `codex`. Trunk
branches (`main`, `master`, `develop`) carry no prefix. Include a ticket number
when one exists.

Examples: `feature/add-openwrt-ci-cache`, `fix/kuiper-driver-build`,
`chore/update-agent-skills`.

### Commit messages

Format `<type>[optional scope][!]: <subject>`. Subject in imperative mood,
lowercase after the colon, no trailing period. Types: `feat`, `fix`, `docs`,
`style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`, `revert`. Add a
scope when it helps (`fix(driver): ...`). Mark a breaking change with `!` before
the colon and/or a `BREAKING CHANGE:` footer. `feat` maps to a MINOR bump, `fix`
to PATCH, and a breaking change to MAJOR.

Examples: `ci: add Kuiper build workflow`,
`fix(driver): keep legacy ieee80211_ops signatures for adi builds`,
`docs(openwrt): update build instructions`.

### Versioning

Use Semantic Versioning `MAJOR.MINOR.PATCH`: a breaking change bumps MAJOR, a
backward-compatible feature bumps MINOR, and a backward-compatible fix bumps
PATCH. Pre-release (`-rc.1`, `-alpha.1`) and build metadata (`+build.5`) are
optional suffixes. A leading `v` is a tag-name convention only (`git tag
v1.5.0`); the version itself is `1.5.0`.
