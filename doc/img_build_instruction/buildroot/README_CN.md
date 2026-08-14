# OpenWiFi Buildroot 镜像

[English documentation](README.md)


## 1. 项目目标与当前状态

本移植为 OpenWiFi 的 Zynq-7000 + AD9361 设备提供一个小型、可重复构建、可从 SD 卡快速启动的 Buildroot 系统，用来替代体积为数 GiB 的完整 Linux 发行版镜像。

当前支持并已在实物上完成启动和 OpenWiFi 基本功能验证的设备如下：

| 构建名称 | 硬件 | 串口控制台 | 内存 |
| --- | --- | --- | --- |
| `antsdr_e200` | ANTSDR-E200 | `ttyPS0` | 512 MiB |
| `antsdr` | ANTSDR-E310/ANT | `ttyPS0` | 1 GiB |
| `e310v2` | ANTSDR-E310V2 | `ttyPS0` | 1 GiB |

三种设备共用同一个 Linux 内核、内核模块集合和 ext4 根文件系统；SPL/U-Boot、设备树、FPGA bitstream、PS 初始化代码和串口路径等硬件差异保留在各自的 BOOT 内容中。

这是新增的并行构建方式。原有 ADI Kuiper 大容量 SD 卡镜像流程和 OpenWrt 流程仍然保留，并继续使用原有脚本、文档和输出目录。Buildroot 不会调用或替换 `user_space/update_sdcard.sh`、`user_space/prepare_kernel.sh`、`user_space/boot_bin_gen.sh` 或 `kernel_boot/build_boot_bin.sh`。

当前默认镜像布局为：

```text
SD card
├── partition 1: 64 MiB FAT32, label BOOT
└── partition 2: 96 MiB ext4, label rootfs
```

完整 SD 卡镜像约 169 MB；包含完整根文件系统的压缩在线更新包通常约 20 MB，实际大小会随 OpenWiFi 和软件包内容变化。

主要版本基线：

| 组件 | 版本/来源 |
| --- | --- |
| Buildroot | `2025.02.11` 子模块 |
| Linux | Analog Devices Linux 6.12，提交 `40201abd70d8f7848547a09fc7e366fba064483c` |
| U-Boot | Xilinx `xlnx_rebase_v2024.01_2024.2` |
| libad9361-iio | `v0.3` |
| OpenWiFi | 构建时的当前仓库源码和短 Git revision |

## 2. 已完成的主要修改

### 2.1 Buildroot 与仓库边界

- 将 Buildroot `2025.02.11` 作为固定版本的 Git 子模块加入 `buildroot/`。
- Buildroot 子模块保持原样，不在其中保存任何 OpenWiFi 私有修改。
- 所有板级配置、软件包、补丁、rootfs overlay 和镜像生成逻辑都位于 `buildroot-external/`。
- 新增统一入口 `buildroot-build.sh`，负责公共系统构建和各设备镜像组装。
- 增加基于 `curl` 的 Buildroot 下载适配器以及 GNU 镜像地址回退，下载内容仍由 Buildroot hash 校验。
- `output/` 和 `dl/` 分别作为生成目录与下载缓存，不进入 Git 跟踪。

预期仓库边界为：

```text
buildroot/            未修改的 Buildroot Git 子模块
buildroot-external/   OpenWiFi Buildroot 外部树
buildroot-build.sh    多设备构建入口
host-tools/           上位机固件更新工具
output/               构建结果，Git 忽略
dl/                   公共下载缓存，Git 忽略
```

### 2.2 公共系统与分板 BOOT 架构

- `output/common/` 中只构建一次 ARMv7 hard-float 工具链、Linux 6.12、OpenWiFi 模块和公共 rootfs。
- 公共内核同时编译 `antsdr_e200.dtb`、`antsdr.dtb` 和 `e310v2.dtb`。
- `output/<board>/` 只组装对应设备的 BOOT 文件、SD 卡镜像和完整系统更新包。
- 从设备对应的 `system_top.xsa` 中提取 FPGA bitstream 以及 `ps7_init_gpl.c/h`，避免混用不同硬件设计。
- 构建时生成 U-Boot DTS 和 PS7 初始化补丁，不修改 OpenWiFi 仓库中已有的板级 DTS。

### 2.3 启动链与 BOOT 分区

BOOT 分区沿用 OpenWiFi 官方镜像中容易识别的命名：

```text
BOOT.BIN
u-boot.img
uEnv.txt
uImage
devicetree.dtb
system_top.bit.bin
```

本镜像的 `BOOT.BIN` 是 U-Boot SPL，而不是官方 Kuiper 镜像中由 Xilinx FSBL 组成的复合启动文件，因此必须同时保留 `u-boot.img`。不能只依据文件名互换两种镜像中的 `BOOT.BIN`。

启动过程如下：

```text
BootROM
  -> BOOT.BIN (U-Boot SPL + board PS7 initialization)
  -> u-boot.img
  -> import uEnv.txt
  -> load system_top.bit.bin into FPGA
  -> load uImage and devicetree.dtb
  -> mount /dev/mmcblk0p2 as ext4 rootfs
```

相关修正包括：

- 为 U-Boot 2024 更新 SPL 使用的 `bootph-all` 标记，确保 UART、SD/MMC、SLCR、时钟和定时器节点不会被 SPL 设备树裁剪。
- 构建完成时自动检查 SPL 中的串口、MMC、TWD timer 和 `stdout-path`，发现错误时拒绝生成镜像。
- 修正三块板的实际 UART 路径：E200 使用 `e0000000`，ANTSDR/E310 和 E310V2 使用 `e0001000`。
- 去掉公共内核中绑定单一 UART 的 `DEBUG_LL`，使同一个内核可以安全用于三种设备。
- E200 增加 SPL MMC/复位诊断补丁，用于定位早期启动复位循环。
- U-Boot 在启动阶段加载 FPGA，Linux 启动后无需再次破坏已经工作的 AD9361/IIO 链路。

### 2.4 Linux、AD9361 与 OpenWiFi

- 使用 Analog Devices Linux 6.12 源码和 OpenWiFi 已有的 AD9361/AXI 补丁。
- 内核版本增加 `-openwifi` 后缀，并关闭不需要的 DRM、声音、媒体和调试信息以减小镜像。
- 将 `cfg80211`、`mac80211` 和 Xilinx DMA 作为模块，在根文件系统可用后由用户空间加载。
- 修正 AD9361 debugfs 表容量，使当前 AD9361 补丁组合可以正确构建和运行。
- OpenWiFi 驱动、side-channel 模块、脚本以及 `sdrctl`、`side_ch_ctl`、`inject_80211` 和 `analyze_80211` 被安装到 `/root/openwifi`。
- 针对精简系统调整官方用户脚本：用 `modprobe` 复用系统中的 AD9361/Xilinx DMA 模块，去掉目标板上不需要的 `sudo`，将 Debian `service` 调用改为 Buildroot init 脚本，并以 BusyBox `httpd` 替代不存在的 `webfsd`。
- `wgd.sh` 会确认 AD9361、DDS、ADC 已就绪，挂载 debugfs，并保证 OpenWiFi 网卡最终命名为 `sdr0`。
- 关闭内核的 mac80211 verbose/HT 调试菜单，避免预期的 BA 协商结果持续刷满控制台；debugfs 仍然保留用于诊断。

### 2.5 FPGA Manager 动态加载

正常启动时 FPGA 已由 U-Boot 加载，推荐只重载驱动：

```sh
cd /root/openwifi
OPENWIFI_RELOAD_FPGA=0 ./wgd.sh 0
```

需要调试一个拓扑兼容的 bitstream 时，可以通过 Linux FPGA Manager 强制重载：

```sh
cd /root/openwifi
OPENWIFI_RELOAD_FPGA=1 ./wgd.sh 0
```

为避免动态重载期间 AXI/IIO 链路卡死，脚本进行了以下修改：

- 重载前停止 `iiod`，并解绑 AD9361、CF AXI DDS 和 CF AXI ADC 消费者。
- 不解绑 E200 上可能在 remove/probe 阶段访问已失效 PL 寄存器的 Xilinx VDMA 设备。
- FPGA Manager 写入成功后主动重新绑定 AD9361、DDS 和 ADC。
- 如果原来运行了 `iiod`，重绑定完成后恢复服务。
- 所有 sysfs 等待都有有限超时，默认 50 次、每次 0.2 秒；可通过 `OPENWIFI_FPGA_WAIT_RETRIES` 调整，不会再无限等待不存在的设备。
- 正确解析设备树中以 NUL 分隔的 `compatible` 字符串，消除 shell 的 null-byte 警告。

动态重载要求新旧 FPGA 设计的地址和设备拓扑兼容。对于普通 AP 或吞吐量测试，不需要动态重载 FPGA。

自动策略会区分系统类型：当射频链路已就绪时，Buildroot 复用 U-Boot 配置的 FPGA；Kuiper 和 OpenWrt 则保持原有的默认行为，继续重载本地 FPGA 镜像。显式设置 `OPENWIFI_RELOAD_FPGA=0/1` 始终具有最高优先级。`iiod` 停止/恢复逻辑同时兼容 Buildroot SysV init 和 Kuiper systemd 服务。

### 2.6 根文件系统内容

公共 rootfs 使用 musl，并包含：

- OpenWiFi 内核模块、脚本和用户空间工具；
- `hostapd`、`wpa_supplicant`、ISC DHCP server/client；
- `iw`、`iwconfig`、`iproute2`、`ethtool`；
- `iperf3`、`tcpdump`；
- `libiio`、`iiod` 及 IIO 测试工具；
- 新增的 `libad9361-iio` Buildroot 外部软件包；
- Dropbear SSH；
- 带 OpenWiFi 字符标识、官方链接和项目简介的 `/etc/motd`。

系统启动时根据 `/proc/device-tree/model` 生成 `/etc/openwifi-board`，因此公共 rootfs 不需要写死设备型号。
BOOT 分区正常以只读方式挂载，`/tmp` 和 `/run` 使用 tmpfs；首次启动生成的 Dropbear 主机密钥保存在 BOOT 分区，重启和完整系统更新后继续使用同一 SSH 身份。

OpenWiFi application notes 中需要直接控制驱动、网卡、IIO 或 FPGA 的 shell 程序在板上运行；绘图、波形显示、离线数据分析以及文档中明确要求 MATLAB/Python GUI 的程序仍然在上位机运行。Buildroot 移植提供了板端运行环境，并不把所有上位机可视化软件塞入 rootfs。

默认配置：

```text
root password: openwifi
eth0:          192.168.10.122/24
serial:        115200 8N1
OpenWiFi dir:  /root/openwifi
```

### 2.7 完整系统在线更新

每个设备都会生成一个包含 BOOT 全部文件和完整 ext4 rootfs 的 `.frm` 包。更新方案只使用标准 SSH/SCP，不运行额外的更新守护进程，不需要端口或 Token。

更新器会：

1. 检查包格式和目标板型号；
2. 检查每个文件的长度和 SHA-256；
3. 使用 `gzip -dc` 与普通 `tar` 分步解包，以兼容未启用 `tar -z` 的 BusyBox，并验证完整 ext4 镜像的长度和 SHA-256；
4. 将 BOOT 文件暂存后按安全顺序替换；
5. 保留 Dropbear 主机密钥，避免更新后 SSH 身份变化；
6. 将当前 rootfs 重新挂载为只读；
7. 流式写入 `/dev/mmcblk0p2`；
8. 使用提前放在 tmpfs 中的重启程序或 Magic SysRq 立即重启；
9. 上位机看到完成标记后主动断开旧 SSH 会话，避免板卡已经重启而命令仍长时间不退出。

该方案为单 rootfs 更新，没有 A/B 回滚副本。写入期间必须保持供电。

## 3. 获取源码与准备环境

在 Ubuntu/Debian 主机上安装 Buildroot 常用依赖：

```sh
sudo apt install build-essential git rsync cpio unzip bc file wget curl python3 \
    libncurses-dev
```

克隆 OpenWiFi 后初始化 Buildroot 子模块：

```sh
git submodule update --init buildroot
```

默认从相邻的硬件镜像仓库中选择匹配的 XSA：

```text
../openwifi-hw-img/boards/<board>/sdk/system_top.xsa
```

也可以指定硬件仓库或单个 XSA：

```sh
OPENWIFI_HW_IMG_DIR=/path/to/openwifi-hw-img \
    ./buildroot-build.sh antsdr_e200 build

OPENWIFI_XSA=/path/to/system_top.xsa \
    ./buildroot-build.sh antsdr_e200 build
```

必须使用与目标设备和 FPGA 设计匹配的 XSA。

## 4. 构建镜像

在 OpenWiFi 仓库根目录执行：

```sh
./buildroot-build.sh antsdr_e200 build
./buildroot-build.sh antsdr build
./buildroot-build.sh e310v2 build
```

第一次构建会创建公共系统，后续设备复用相同内核和 rootfs，只重建该设备的 U-Boot 和镜像。

主要产物：

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

其他构建命令：

```sh
./buildroot-build.sh antsdr_e200 configure
./buildroot-build.sh antsdr_e200 menuconfig
./buildroot-build.sh antsdr_e200 rebuild-system
./buildroot-build.sh antsdr_e200 clean
```

- `configure`：重新生成板级输入并加载公共 defconfig。
- `menuconfig`：修改公共 Buildroot 配置，并将公共系统标记为需要重建。
- `rebuild-system`：修改内核、OpenWiFi、外部软件包或 rootfs overlay 后，保留工具链并强制重建公共系统。
- `clean`：只删除所选设备的输出，保留 `output/common/` 和下载缓存。

## 5. 首次烧录 SD 卡

首次安装必须烧录完整 `.img`，不能只把 BOOT 文件复制到一张没有正确分区和 ext4 rootfs 的卡中。

先确认设备名：

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINTS
```

卸载该卡上已经自动挂载的分区，然后写入整个设备，例如 E200：

```sh
sudo umount /dev/sdX1 /dev/sdX2 2>/dev/null || true
sudo dd if=output/antsdr_e200/images/openwifi-antsdr_e200-sdcard.img \
    of=/dev/sdX bs=4M conv=fsync status=progress
sync
```

将 `/dev/sdX` 替换为确认无误的 SD 卡整盘设备。该操作会覆盖目标设备上的数据，不能写到 `/dev/sdX1`。

## 6. 启动和基本使用

使用 115200 8N1 连接串口并启动。登录信息：

```text
login:    root
password: openwifi
```

检查板型、启动参数和 IIO：

```sh
cat /etc/openwifi-board
cat /proc/cmdline
iio_info -s
```

加载正常 OpenWiFi 栈：

```sh
openwifi-start 0
```

等价于：

```sh
cd /root/openwifi
./wgd.sh 0
```

检查结果：

```sh
ip link show sdr0
iw dev
dmesg | tail -100
```

目前经过稳定性验证的默认方式是 `test_mode=0`。只有明确验证某个 FPGA/驱动组合的聚合功能时才切换其他 test mode。

## 7. 通过网口更新完整系统

设备与主机的管理网口连通后，先查看状态：

```sh
./host-tools/openwifi_fw_update.py --host 192.168.10.122 status
```

在板上完整校验固件包：

```sh
./host-tools/openwifi_fw_update.py --host 192.168.10.122 check \
    output/antsdr_e200/images/openwifi-antsdr_e200-system.frm
```

更新 BOOT 和完整 ext4 rootfs，并立即重启：

```sh
./host-tools/openwifi_fw_update.py --host 192.168.10.122 update --reboot \
    output/antsdr_e200/images/openwifi-antsdr_e200-system.frm
```

密码默认为 `openwifi`。工具使用普通 SSH 登录，将固件和板端更新脚本临时复制到 `/tmp`，完成后清理。设备型号不匹配时会拒绝更新。

也可以手动执行：

```sh
scp output/antsdr_e200/images/openwifi-antsdr_e200-system.frm \
    root@192.168.10.122:/tmp/openwifi-system.frm
ssh root@192.168.10.122
openwifi-fw-update status
openwifi-fw-update check /tmp/openwifi-system.frm
openwifi-fw-update install --reboot /tmp/openwifi-system.frm
```

因为更新器覆盖当前挂载的根文件系统，`install` 必须带 `--reboot`。

## 8. 验证结论与注意事项

截至本文更新时：

- ANTSDR-E200、ANTSDR-E310/ANT 和 ANTSDR-E310V2 均已由用户在实物上验证可以正常启动并运行 OpenWiFi。
- 三种设备确认可以复用公共 `uImage`、模块和 `rootfs.ext4`，同时使用各自的 BOOT 文件。
- E200 已验证 OpenWiFi AP/客户端连接及 `iperf3` 数据传输。
- 原有 Kuiper 构建脚本和说明文件与官方版本保持不变，并使用 2025-03-18 ADI Kuiper 镜像实际跑通了完整的旧 SD 卡准备流程：镜像烧录后完成逐字节校验，成功构建 32/64 位 Linux 6.12 内核与模块，生成 E200 FSBL、BOOT.BIN、设备树和 bitstream，安装两种架构的 OpenWiFi 驱动，最终 FAT/ext4 文件系统检查均通过。
- 两块同时接入同一管理交换机的设备不能都保留默认 `192.168.10.122`，应临时修改其中一块的 `eth0` 地址。
- 普通运行优先使用 U-Boot 已加载的 FPGA，即 `OPENWIFI_RELOAD_FPGA=0`；动态加载只用于兼容 bitstream 的开发调试。
- 完整系统更新为单分区原地更新，不具备断电自动回滚能力。

提交前确认 Buildroot 子模块没有被意外修改：

```sh
git -C buildroot status --short
git submodule status buildroot
```

## 9. 文件级修改索引

| 路径 | 作用 |
| --- | --- |
| `.gitmodules`, `.gitignore`, `.gitattributes` | 注册 Buildroot 子模块、忽略产物/缓存，并声明补丁文件空白规则 |
| `README.md` | 项目主 README 默认链接的英文构建说明 |
| `README_CN.md` | 可选的 Buildroot 中文说明 |
| `buildroot-build.sh` | 校验板型/XSA、维护公共输出、调用 Buildroot 并组装分板镜像 |
| `buildroot-external/Config.in`, `external.desc`, `external.mk` | 定义外部树、板型选项、外部软件包以及三个 Linux DTB 的安装/串口修正 |
| `buildroot-external/configs/openwifi_common_defconfig` | 公共 ARM/Linux/rootfs/U-Boot 和用户空间软件包配置 |
| `buildroot-external/board/common/linux-*.fragment` | 精简公共内核，配置 FPGA/DMA、mac80211 日志和安全重启能力 |
| `buildroot-external/board/common/uboot.fragment` | SPL/U-Boot、FPGA 和 `uEnv.txt` 启动命令配置 |
| `buildroot-external/board/common/genimage.cfg`, `system_top.bif.in`, `uEnv.txt.in` | SD 分区、bitstream 转换和 BOOT 环境模板 |
| `buildroot-external/board/common/rootfs-overlay/` | fstab、管理网口、MOTD、DHCP 兼容入口、BOOT bitstream 链接以及 SSH 主机密钥持久化 |
| `buildroot-external/board/common/openwifi-start`, `openwifi-board-name`, `S02openwifi-board` | OpenWiFi 启动入口和运行时板型识别 |
| `buildroot-external/board/common/openwifi-fw-update` | 板端完整系统校验、写入和重启程序 |
| `buildroot-external/board/common/curl-wget`, `busybox-openwifi.fragment`, `post-build.sh` | 可靠下载、BusyBox 功能补充和旧实验更新文件清理 |
| `buildroot-external/package/openwifi/` | 构建/安装 OpenWiFi 模块、工具、脚本及 Buildroot 兼容调整 |
| `buildroot-external/package/libad9361-iio/` | 新增 libad9361-iio 外部包、版本 hash 和 CMake 配置 |
| `buildroot-external/patches/linux/` | 修正 AD9361 debugfs 容量 |
| `buildroot-external/patches/uboot/antsdr_e200/` | E200 SPL MMC 与复位状态诊断 |
| `buildroot-external/support/prepare-board.py` | 从 XSA 生成 PS7 补丁、修正 U-Boot DTS、生成分板 `uEnv.txt` |
| `buildroot-external/support/fix-linux-dts-console.py` | 修正公共内核中每块板的 Linux `stdout-path` |
| `buildroot-external/support/assemble-board.sh` | 校验 SPL、生成 bitstream、BOOT FAT、SD 镜像和分板产物 |
| `buildroot-external/support/mk-firmware-package.sh` | 生成带 manifest、长度和 SHA-256 的完整 `.frm` |
| `host-tools/openwifi_fw_update.py` | 通过 SSH/SCP 校验、更新并在板卡重启后可靠退出 |
| `user_space/wgd.sh` | 增加 FPGA 重载策略、RF 链检查、debugfs 和 `sdr0` 命名保障 |
| `user_space/load_fpga_img.sh` | 安全动态重载 FPGA，解绑/重绑 AD9361/IIO，增加有限超时 |
