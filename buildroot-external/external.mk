include $(sort $(wildcard $(BR2_EXTERNAL_OPENWIFI_PATH)/package/*/*.mk))

# Linux 6.12 groups ARM device trees by vendor. Build all supported DTBs in the
# shared system build. U-Boot uses one generated DTS at a time with additional
# SPL boot-phase markers; see support/prepare-board.py.
define OPENWIFI_LINUX_INSTALL_DTS
	mkdir -p $(@D)/arch/arm/boot/dts/xilinx
	cp -f $(BR2_EXTERNAL_OPENWIFI_PATH)/../kernel_boot/boards/antsdr_e200/devicetree.dts \
		$(@D)/arch/arm/boot/dts/xilinx/antsdr_e200.dts
	cp -f $(BR2_EXTERNAL_OPENWIFI_PATH)/../kernel_boot/boards/antsdr/devicetree.dts \
		$(@D)/arch/arm/boot/dts/xilinx/antsdr.dts
	cp -f $(BR2_EXTERNAL_OPENWIFI_PATH)/../kernel_boot/boards/e310v2/devicetree.dts \
		$(@D)/arch/arm/boot/dts/xilinx/e310v2.dts
	$(BR2_EXTERNAL_OPENWIFI_PATH)/support/fix-linux-dts-console.py \
		$(@D)/arch/arm/boot/dts/xilinx/antsdr_e200.dts \
		/amba/serial@e0000000
	$(BR2_EXTERNAL_OPENWIFI_PATH)/support/fix-linux-dts-console.py \
		$(@D)/arch/arm/boot/dts/xilinx/antsdr.dts \
		/amba/serial@e0001000
	$(BR2_EXTERNAL_OPENWIFI_PATH)/support/fix-linux-dts-console.py \
		$(@D)/arch/arm/boot/dts/xilinx/e310v2.dts \
		/axi/serial@e0001000
endef

LINUX_POST_PATCH_HOOKS += OPENWIFI_LINUX_INSTALL_DTS
