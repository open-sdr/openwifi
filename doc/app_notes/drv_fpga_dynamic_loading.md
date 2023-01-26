The **wgd.sh** (running on board) supports reloading driver and/or FPGA image dynamically without rebooting/power-cycle. It can work in a 
flexible way. 

The purpose of this feature is to help you easily reload driver and FPGA built from your branch/version/variant/modification, and switch/run different driver and FPGA of different branch/version/variant/modification without rebooting. To enjoy this feature, always ensure your onboard openwifi/files are the latest files in [user_space](../../user_space)).

- [[Reload driver only](#Reload-driver-only)]
- [[Reload driver and FPGA](#Reload-driver-and-FPGA)]
- [[Reload driver and FPGA in target directory](#Reload-driver-and-FPGA-in-target-directory)]
- [[Reload driver and FPGA from a single package file](#Reload-driver-and-FPGA-from-a-single-package-file)] -- **RECOMMENDED!**
- [[Suggested practice to generate driver FPGA variants](#Suggested-practice-to-generate-driver-FPGA-variants)]
- [[Detailed full usage info](#Detailed-full-usage-info)]

Note: Make sure you have compiled driver before. Check [Update Driver](../../README.md#update-driver).

## Reload driver only
This is the original way. To let **wgd.sh** only loads the driver without touching FPGA, please ensure FPGA image file **system_top.bit.bin** is **NOT** 
present in the directory. If wgd.sh can not find the FPGA image, it will skip reloading it.

## Reload driver and FPGA
- Generate the reloadable FPGA file **system_top.bit.bin**. In the Linux host computer:
  ```
  cd openwifi/user_space
  ./drv_and_fpga_package_gen.sh $OPENWIFI_HW_IMG_DIR $XILINX_DIR $BOARD_NAME
  ```
  Then **system_top.bit.bin** will be generated in openwifi/user_space.

- Put **system_top.bit.bin** on board in the same directory as wgd.sh and other driver files (.ko)
- Run **wgd.sh** on board as usual

## Reload driver and FPGA in target directory
Put **system_top.bit.bin** on board together with other driver files (.ko) in a directory ($TARGET_DIR), then run on board:
```
./wgd.sh $TARGET_DIR
```
In this way, different versions/variants of driver/FPGA can be put in different directories. Then **wgd.sh** can be used to switch 
between them without rebooting/power-cycle.

## Reload driver and FPGA from a single package file
The openwifi/user_space/**drv_and_fpga_package_gen.sh** also generates a single package file **drv_and_fpga.tar.gz**, which includes driver files (.ko), 
FPGA image and many other source files with rich infos that are related.

You can switch to your own branch/version/variant, build the single package file via **drv_and_fpga_package_gen.sh**, rename it with a more meaningful name (such as add version or variant info as postfix), put the renamed **drv_and_fpga_MEANINGFUL_POSTFIX.tar.gz** on board in the same directory as **wgd.sh**, and let **wgd.sh** load it:
```
./wgd.sh ./drv_and_fpga_MEANINGFUL_POSTFIX.tar.gz
```
In this way, different version/variants of driver/FPGA can be switched by **wgd.sh** without rebooting/power-cycle.

## Suggested practice to generate driver FPGA variants
There are several ways to generate variants of the single driver-FPGA package file. For example:

- Switch/create another branch for openwifi and openwifi-hw, work/modify there, then generate the single package file via **drv_and_fpga_package_gen.sh**. This package is the branch specific, so renaming the package name to a more meaningful one would be good practice.
- In the same branch, set different arguments (finally macro definitions in .h and .v files) via conditional compiling to enable/disable different driver and FPGA code blocks/functionalities, then generate the single package file via **drv_and_fpga_package_gen.sh**. Rename the package to remind you which conditions are ON/OFF.
    - Check "Conditional compile by verilog macro" in openwifi-hw README for FPGA design
    - Input more arguments (max 5) to driver building script "make_all.sh $XILINX_DIR ARCH_BIT". Those arguments will be converted to "#define argument" in pre_def.h for driver conditional compiling. **Attention:** **drv_and_fpga_package_gen.sh** currently only call **make_all.sh** without extra arguments. If you have conditional compiling arguments, do not forget to put them into **drv_and_fpga_package_gen.sh** as extra arguments of **make_all.sh**.

## Detailed full usage info
Run the "./wgd.sh -h" on board or open wgd.sh to see full usage info:
```
usage:
  Script for load (or download+load) different driver and FPGA img without rebooting
  no  argument: Load .ko driver files and FPGA img (if system_top.bit.bin exist) in current dir with test_mode=0.
  1st argument: If it is a NUMBER, it will be assigned to test_mode. Then load everything from current dir.
  1st argument: If it is a string called "remote", it will download driver/FPGA and load everything.
  - 2nd argument (if exist) is the target directory name for downloading and reloading
  - 3rd argument (if exist) is the value for test_mode
  1st argument: neither NUMBER nor "remote" nor a .tar.gz file, it is regarded as a directory and load everything from it.
  - 2nd argument (if exist) is the value for test_mode
  1st argument: a .tar.gz file, it will be unpacked then load from that unpacked directory
  - 2nd argument (if exist) is the value for test_mode
```
