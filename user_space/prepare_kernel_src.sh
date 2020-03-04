  
#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "You must enter exactly 2 arguments: \$OPENWIFI_DIR \$XILINX_DIR"
    exit 1
fi

OPENWIFI_DIR=$1
XILINX_DIR=$2


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

home_dir=$(pwd)

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

cd $home_dir
