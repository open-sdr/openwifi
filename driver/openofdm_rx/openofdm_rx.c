/*
 * Author: Xianjun jiao, Michael Mehari, Wei Liu
 * SPDX-FileCopyrightText: 2019 UGent
 * SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include <linux/bitops.h>
#include <linux/dmapool.h>
#include <linux/dma/xilinx_dma.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_dma.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/delay.h>

#include "../hw_def.h"

static void __iomem *base_addr; // to store driver specific base address needed for mmu to translate virtual address to physical address in our FPGA design

/* IO accessors */
static inline u32 reg_read(u32 reg)
{
	return ioread32(base_addr + reg);
}

static inline void reg_write(u32 reg, u32 value)
{
	iowrite32(value, base_addr + reg);
}

static inline u32 OPENOFDM_RX_REG_STATE_HISTORY_read(void){
	return reg_read(OPENOFDM_RX_REG_STATE_HISTORY_ADDR);
}

static inline void OPENOFDM_RX_REG_MULTI_RST_write(u32 Data) {
	reg_write(OPENOFDM_RX_REG_MULTI_RST_ADDR, Data);
}
static inline void OPENOFDM_RX_REG_ENABLE_write(u32 Data) {
	reg_write(OPENOFDM_RX_REG_ENABLE_ADDR, Data);
}
static inline void OPENOFDM_RX_REG_POWER_THRES_write(u32 Data) {
	reg_write(OPENOFDM_RX_REG_POWER_THRES_ADDR, Data);
}
static inline void OPENOFDM_RX_REG_MIN_PLATEAU_write(u32 Data) {
	reg_write(OPENOFDM_RX_REG_MIN_PLATEAU_ADDR, Data);
}
static inline void OPENOFDM_RX_REG_SOFT_DECODING_write(u32 Data) {
	reg_write(OPENOFDM_RX_REG_SOFT_DECODING_ADDR, Data);
}
static inline void OPENOFDM_RX_REG_FFT_WIN_SHIFT_write(u32 Data) {
	reg_write(OPENOFDM_RX_REG_FFT_WIN_SHIFT_ADDR, Data);
}
static inline void OPENOFDM_RX_REG_PHASE_OFFSET_ABS_TH_write(u32 Data) {
	reg_write(OPENOFDM_RX_REG_PHASE_OFFSET_ABS_TH_ADDR, Data);
}
static const struct of_device_id dev_of_ids[] = {
	{ .compatible = "sdr,openofdm_rx", },
	{}
};
MODULE_DEVICE_TABLE(of, dev_of_ids);

static struct openofdm_rx_driver_api openofdm_rx_driver_api_inst;
struct openofdm_rx_driver_api *openofdm_rx_api = &openofdm_rx_driver_api_inst;
EXPORT_SYMBOL(openofdm_rx_api);

static inline u32 hw_init(enum openofdm_rx_mode mode){
	int err=0, i;

	printk("%s hw_init mode %d\n", openofdm_rx_compatible_str, mode);

	switch(mode)
	{
		case OPENOFDM_RX_TEST:
		{
			printk("%s hw_init mode OPENOFDM_RX_TEST\n", openofdm_rx_compatible_str);
			break;
		}
		case OPENOFDM_RX_NORMAL:
		{
			printk("%s hw_init mode OPENOFDM_RX_NORMAL\n", openofdm_rx_compatible_str);
			break;
		}
		default:
		{
			printk("%s hw_init mode %d is wrong!\n", openofdm_rx_compatible_str, mode);
			err=1;
		}
	}
	printk("%s hw_init input: power_thres %d dc_running_sum_th %d min_plateau %d\n", openofdm_rx_compatible_str, OPENOFDM_RX_POWER_THRES_INIT, OPENOFDM_RX_DC_RUNNING_SUM_TH_INIT, OPENOFDM_RX_MIN_PLATEAU_INIT);

	// 1) power threshold configuration and reset
  openofdm_rx_api->OPENOFDM_RX_REG_ENABLE_write(1); //bit1 of slv_reg1: force ht smoothing to have better sensitivity
	
  // Remove OPENOFDM_RX_REG_POWER_THRES_write to avoid hw_init call in openwifi_start causing inconsistency
  // openofdm_rx_api->OPENOFDM_RX_REG_POWER_THRES_write((OPENOFDM_RX_DC_RUNNING_SUM_TH_INIT<<16)|OPENOFDM_RX_POWER_THRES_INIT); // turn on signal watchdog by default
	
  openofdm_rx_api->OPENOFDM_RX_REG_MIN_PLATEAU_write(OPENOFDM_RX_MIN_PLATEAU_INIT);
	openofdm_rx_api->OPENOFDM_RX_REG_SOFT_DECODING_write((OPENWIFI_MAX_SIGNAL_LEN_TH<<16)|(OPENWIFI_MIN_SIGNAL_LEN_TH<<12)|1); //bit1 enable soft decoding; bit15~12 min pkt length threshold; bit31~16 max pkt length threshold
	openofdm_rx_api->OPENOFDM_RX_REG_FFT_WIN_SHIFT_write((OPENOFDM_RX_SMALL_EQ_OUT_COUNTER_TH<<4)|OPENOFDM_RX_FFT_WIN_SHIFT_INIT);
  openofdm_rx_api->OPENOFDM_RX_REG_PHASE_OFFSET_ABS_TH_write(OPENOFDM_RX_PHASE_OFFSET_ABS_TH);

	//rst
	for (i=0;i<8;i++)
		openofdm_rx_api->OPENOFDM_RX_REG_MULTI_RST_write(0);
	for (i=0;i<32;i++)
		openofdm_rx_api->OPENOFDM_RX_REG_MULTI_RST_write(0xFFFFFFFF);
	for (i=0;i<8;i++)
		openofdm_rx_api->OPENOFDM_RX_REG_MULTI_RST_write(0);

	printk("%s hw_init err %d\n", openofdm_rx_compatible_str, err);

	return(err);
}

static int dev_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *io;
	int err=1;

	printk("\n");

	if (np) {
		const struct of_device_id *match;

		match = of_match_node(dev_of_ids, np);
		if (match) {
			printk("%s dev_probe match!\n", openofdm_rx_compatible_str);
			err = 0;
		}
	}

	if (err)
		return err;

	openofdm_rx_api->hw_init=hw_init;

	openofdm_rx_api->reg_read=reg_read;
	openofdm_rx_api->reg_write=reg_write;

	openofdm_rx_api->OPENOFDM_RX_REG_MULTI_RST_write=OPENOFDM_RX_REG_MULTI_RST_write;
	openofdm_rx_api->OPENOFDM_RX_REG_ENABLE_write=OPENOFDM_RX_REG_ENABLE_write;
	openofdm_rx_api->OPENOFDM_RX_REG_POWER_THRES_write=OPENOFDM_RX_REG_POWER_THRES_write;
	openofdm_rx_api->OPENOFDM_RX_REG_MIN_PLATEAU_write=OPENOFDM_RX_REG_MIN_PLATEAU_write;
	openofdm_rx_api->OPENOFDM_RX_REG_SOFT_DECODING_write=OPENOFDM_RX_REG_SOFT_DECODING_write;
	openofdm_rx_api->OPENOFDM_RX_REG_FFT_WIN_SHIFT_write=OPENOFDM_RX_REG_FFT_WIN_SHIFT_write;
	openofdm_rx_api->OPENOFDM_RX_REG_PHASE_OFFSET_ABS_TH_write=OPENOFDM_RX_REG_PHASE_OFFSET_ABS_TH_write;

	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base_addr = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(base_addr))
		return PTR_ERR(base_addr);

	printk("%s dev_probe io start 0x%08x end 0x%08x name %s flags 0x%08x desc 0x%08x\n", openofdm_rx_compatible_str,io->start,io->end,io->name,(u32)io->flags,(u32)io->desc);
	printk("%s dev_probe base_addr 0x%08x\n", openofdm_rx_compatible_str,(u32)base_addr);
	printk("%s dev_probe openofdm_rx_driver_api_inst 0x%08x\n", openofdm_rx_compatible_str, (u32)&openofdm_rx_driver_api_inst);
	printk("%s dev_probe             openofdm_rx_api 0x%08x\n", openofdm_rx_compatible_str, (u32)openofdm_rx_api);

	printk("%s dev_probe succeed!\n", openofdm_rx_compatible_str);

	err = hw_init(OPENOFDM_RX_NORMAL);

	return err;
}

static int dev_remove(struct platform_device *pdev)
{
	printk("\n");

	printk("%s dev_remove base_addr 0x%08x\n", openofdm_rx_compatible_str,(u32)base_addr);
	printk("%s dev_remove openofdm_rx_driver_api_inst 0x%08x\n", openofdm_rx_compatible_str, (u32)&openofdm_rx_driver_api_inst);
	printk("%s dev_remove             openofdm_rx_api 0x%08x\n", openofdm_rx_compatible_str, (u32)openofdm_rx_api);

	printk("%s dev_remove succeed!\n", openofdm_rx_compatible_str);
	return 0;
}

static struct platform_driver dev_driver = {
	.driver = {
		.name = "sdr,openofdm_rx",
		.owner = THIS_MODULE,
		.of_match_table = dev_of_ids,
	},
	.probe = dev_probe,
	.remove = dev_remove,
};

module_platform_driver(dev_driver);

MODULE_AUTHOR("Xianjun Jiao");
MODULE_DESCRIPTION("sdr,openofdm_rx");
MODULE_LICENSE("GPL v2");
