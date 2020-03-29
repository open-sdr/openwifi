  
#!/bin/bash
if [ "$#" -ne 4 ]; then
    echo "You have input $# arguments."
    echo "You must enter exactly 4 arguments: \$OPENWIFI_DIR \$XILINX_DIR \$BOARD_NAME \$SDCARD_DIR"
    exit 1
fi

OPENWIFI_DIR=$1
XILINX_DIR=$2
BOARD_NAME=$3
SDCARD_DIR=$4

if [ -f "$OPENWIFI_DIR/LICENSE" ]; then
    echo "\$OPENWIFI_DIR is found!"
else
    echo "\$OPENWIFI_DIR is not correct. Please check!"
    exit 1
fi

if [ -d "$XILINX_DIR/SDK" ]; then
    echo "\$XILINX_DIR is found!"
else
    echo "\$XILINX_DIR is not correct. Please check!"
    exit 1
fi

if [ "$BOARD_NAME" != "zc706_fmcs2" ] && [ "$BOARD_NAME" != "zc702_fmcs2" ] && [ "$BOARD_NAME" != "zed_fmcs2" ] && [ "$BOARD_NAME" != "adrv9361z7035" ] && [ "$BOARD_NAME" != "adrv9361z7035_fmc" ] && [ "$BOARD_NAME" != "adrv9364z7020" ]; then
    echo "\$BOARD_NAME is not correct. Please check!"
    exit 1
else
    echo "\$BOARD_NAME is found!"
fi

# detect SD card mounting status
if [ -d "$SDCARD_DIR/BOOT/" ]; then
    echo "$SDCARD_DIR/BOOT/"
    mkdir $SDCARD_DIR/BOOT/openwifi
else
    echo "$SDCARD_DIR/BOOT/ does not exist!"
    exit 1
fi

if [ -d "$SDCARD_DIR/rootfs/" ]; then
    echo "$SDCARD_DIR/rootfs/"
else
    echo "$SDCARD_DIR/rootfs/ does not exist!"
    exit 1
fi

sudo ls

home_dir=$(pwd)

set -x

if [ -f "$OPENWIFI_DIR/adi-linux/arch/arm/boot/uImage" ]; then
    echo "Skip the time costly Linux kernel compiling."
else
    # Build the Linux kernel uImage and modules
    cd $OPENWIFI_DIR/
    git submodule init adi-linux
    git submodule update adi-linux
    cd adi-linux
    git reset --hard 4220d5d24c6c7589fc702db4f941f0632b5ad767
    cp ../kernel_boot/kernel_config ./.config
    source $XILINX_DIR/SDK/2017.4/settings64.sh
    export ARCH=arm
    export CROSS_COMPILE=arm-linux-gnueabihf-
    make oldconfig && make prepare && make modules_prepare
    make -j12 UIMAGE_LOADADDR=0x8000 uImage
    make modules
fi

$OPENWIFI_DIR/user_space/get_fpga.sh $OPENWIFI_DIR

BOARD_NAME_ALL="zc706_fmcs2 zed_fmcs2 zc702_fmcs2 adrv9361z7035 adrv9361z7035_fmc adrv9364z7020"
for BOARD_NAME_TMP in $BOARD_NAME_ALL
do
# if [ -f "$OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN" ]; then
#     echo "Skip the BOOT.BIN generation."
# else
#     # Build BOOT.BIN
    $OPENWIFI_DIR/user_space/boot_bin_gen.sh $OPENWIFI_DIR $XILINX_DIR $BOARD_NAME_TMP
# fi
    dtc -I dts -O dtb -o $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME_TMP/devicetree.dtb $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME_TMP/devicetree.dts
    mkdir $SDCARD_DIR/BOOT/openwifi/$BOARD_NAME_TMP
    cp $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME_TMP/devicetree.dtb $SDCARD_DIR/BOOT/openwifi/$BOARD_NAME_TMP
    cp $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME_TMP/output_boot_bin/BOOT.BIN $SDCARD_DIR/BOOT/openwifi/$BOARD_NAME_TMP
done

# Copy uImage BOOT.BIN and devicetree to SD card BOOT partition
cp $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME/devicetree.dtb $SDCARD_DIR/BOOT/
cp $OPENWIFI_DIR/kernel_boot/boards/$BOARD_NAME/output_boot_bin/BOOT.BIN $SDCARD_DIR/BOOT/
cp $OPENWIFI_DIR/adi-linux/arch/arm/boot/uImage $SDCARD_DIR/BOOT/

# build openwifi driver
$OPENWIFI_DIR/driver/make_all.sh $OPENWIFI_DIR $XILINX_DIR

# Copy files to SD card rootfs partition
sudo mkdir $SDCARD_DIR/rootfs/root/openwifi
sudo find $OPENWIFI_DIR/driver -name \*.ko -exec cp {} $SDCARD_DIR/rootfs/root/openwifi/ \;
sudo cp $OPENWIFI_DIR/user_space/* $SDCARD_DIR/rootfs/root/openwifi/ -rf
sudo wget -P $SDCARD_DIR/rootfs/root/openwifi/webserver/ https://users.ugent.be/~xjiao/openwifi-low-aac.mp4

sudo mkdir $SDCARD_DIR/rootfs/lib/modules
sudo mkdir $SDCARD_DIR/rootfs/lib/modules/openwifi
sudo find $OPENWIFI_DIR/adi-linux -name \*.ko -exec cp {} $SDCARD_DIR/rootfs/lib/modules/openwifi/ \;
sudo rm $SDCARD_DIR/rootfs/lib/modules/openwifi/{axidmatest.ko,xilinx_dma.ko,adi_axi_hdmi.ko,ad9361_drv.ko} -f

sudo rm $SDCARD_DIR/rootfs/etc/udev/rules.d/70-persistent-net.rules
sudo cp $OPENWIFI_DIR/kernel_boot/70-persistent-net.rules $SDCARD_DIR/rootfs/etc/udev/rules.d/
sudo mv $SDCARD_DIR/rootfs/lib/udev/rules.d/75-persistent-net-generator.rules $SDCARD_DIR/rootfs/lib/udev/rules.d/75-persistent-net-generator.rules.bak

# Some setup
sudo echo -e "\nauto lo eth0\niface lo inet loopback\niface eth0 inet static\naddress 192.168.10.122\nnetmask 255.255.255.0\n" | sudo tee -a $SDCARD_DIR/rootfs/etc/network/interfaces
sudo echo -e "\nnameserver 8.8.8.8\nnameserver 4.4.4.4\n" | sudo tee -a $SDCARD_DIR/rootfs/etc/resolv.conf
sudo echo -e "\nUseDNS no\n" | sudo tee -a $SDCARD_DIR/rootfs/etc/ssh/sshd_config
sudo echo -e "\nnet.ipv4.ip_forward=1\n" | sudo tee -a $SDCARD_DIR/rootfs/etc/sysctl.conf
sudo chmod -x $SDCARD_DIR/rootfs/etc/update-motd.d/90-updates-available
sudo chmod -x $SDCARD_DIR/rootfs/etc/update-motd.d/91-release-upgrade

cd $SDCARD_DIR/BOOT
sync
cd $SDCARD_DIR/rootfs
sync

cd $home_dir

umount $SDCARD_DIR/BOOT/
umount $SDCARD_DIR/rootfs/
