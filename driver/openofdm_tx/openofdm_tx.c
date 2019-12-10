/*
 * axi lite register access driver
 * Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be
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

static inline void OPENOFDM_TX_REG_MULTI_RST_write(u32 Data) {
	reg_write(OPENOFDM_TX_REG_MULTI_RST_ADDR, Data);
}

static inline void OPENOFDM_TX_REG_INIT_PILOT_STATE_write(u32 Data) {
	reg_write(OPENOFDM_TX_REG_INIT_PILOT_STATE_ADDR, Data);
}

static inline void OPENOFDM_TX_REG_INIT_DATA_STATE_write(u32 Data) {
	reg_write(OPENOFDM_TX_REG_INIT_DATA_STATE_ADDR, Data);
}

static const struct of_device_id dev_of_ids[] = {
	{ .compatible = "sdr,openofdm_tx", },
	{}
};
MODULE_DEVICE_TABLE(of, dev_of_ids);

static struct openofdm_tx_driver_api openofdm_tx_driver_api_inst;
static struct openofdm_tx_driver_api *openofdm_tx_api = &openofdm_tx_driver_api_inst;
EXPORT_SYMBOL(openofdm_tx_api);

static inline u32 hw_init(enum openofdm_tx_mode mode){
	int err=0;

	printk("%s hw_init mode %d\n", openofdm_tx_compatible_str, mode);

	switch(mode)
	{
		case OPENOFDM_TX_TEST:
			printk("%s hw_init mode OPENOFDM_TX_TEST\n", openofdm_tx_compatible_str);
			break;

		case OPENOFDM_TX_NORMAL:
			printk("%s hw_init mode OPENOFDM_TX_NORMAL\n", openofdm_tx_compatible_str);
			break;

		default:
			printk("%s hw_init mode %d is wrong!\n", openofdm_tx_compatible_str, mode);
			err=1;
	}

	openofdm_tx_api->OPENOFDM_TX_REG_MULTI_RST_write(0xFFFFFFFF);
	openofdm_tx_api->OPENOFDM_TX_REG_MULTI_RST_write(0xFFFFFFFF);
	openofdm_tx_api->OPENOFDM_TX_REG_MULTI_RST_write(0xFFFFFFFF);
	openofdm_tx_api->OPENOFDM_TX_REG_MULTI_RST_write(0xFFFFFFFF);
	openofdm_tx_api->OPENOFDM_TX_REG_MULTI_RST_write(0xFFFFFFFF);
	openofdm_tx_api->OPENOFDM_TX_REG_MULTI_RST_write(0xFFFFFFFF);
	openofdm_tx_api->OPENOFDM_TX_REG_MULTI_RST_write(0);

	openofdm_tx_api->OPENOFDM_TX_REG_INIT_PILOT_STATE_write(0x7E);
	openofdm_tx_api->OPENOFDM_TX_REG_INIT_DATA_STATE_write(0x7F);

	printk("%s hw_init err %d\n", openofdm_tx_compatible_str, err);
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
			printk("%s dev_probe match!\n", openofdm_tx_compatible_str);
			err = 0;
		}
	}

	if (err)
		return err;

	openofdm_tx_api->hw_init=hw_init;

	openofdm_tx_api->reg_read=reg_read;
	openofdm_tx_api->reg_write=reg_write;

	openofdm_tx_api->OPENOFDM_TX_REG_MULTI_RST_write=OPENOFDM_TX_REG_MULTI_RST_write;
	openofdm_tx_api->OPENOFDM_TX_REG_INIT_PILOT_STATE_write=OPENOFDM_TX_REG_INIT_PILOT_STATE_write;
	openofdm_tx_api->OPENOFDM_TX_REG_INIT_DATA_STATE_write=OPENOFDM_TX_REG_INIT_DATA_STATE_write;
	
	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base_addr = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(base_addr))
		return PTR_ERR(base_addr);

	printk("%s dev_probe io start 0x%08x end 0x%08x name %s flags 0x%08x desc 0x%08x\n", openofdm_tx_compatible_str,io->start,io->end,io->name,(u32)io->flags,(u32)io->desc);
	printk("%s dev_probe base_addr 0x%08x\n", openofdm_tx_compatible_str,(u32)base_addr);
	printk("%s dev_probe openofdm_tx_driver_api_inst 0x%08x\n", openofdm_tx_compatible_str, (u32)&openofdm_tx_driver_api_inst);
	printk("%s dev_probe             openofdm_tx_api 0x%08x\n", openofdm_tx_compatible_str, (u32)openofdm_tx_api);

	printk("%s dev_probe succeed!\n", openofdm_tx_compatible_str);

	err = hw_init(OPENOFDM_TX_NORMAL);

	return err;
}

static int dev_remove(struct platform_device *pdev)
{
	printk("\n");

	printk("%s dev_remove base_addr 0x%08x\n", openofdm_tx_compatible_str,(u32)base_addr);
	printk("%s dev_remove openofdm_tx_driver_api_inst 0x%08x\n", openofdm_tx_compatible_str, (u32)&openofdm_tx_driver_api_inst);
	printk("%s dev_remove             openofdm_tx_api 0x%08x\n", openofdm_tx_compatible_str, (u32)openofdm_tx_api);

	printk("%s dev_remove succeed!\n", openofdm_tx_compatible_str);
	return 0;
}

static struct platform_driver dev_driver = {
	.driver = {
		.name = "sdr,openofdm_tx",
		.owner = THIS_MODULE,
		.of_match_table = dev_of_ids,
	},
	.probe = dev_probe,
	.remove = dev_remove,
};

module_platform_driver(dev_driver);

MODULE_AUTHOR("Xianjun Jiao");
MODULE_DESCRIPTION("sdr,openofdm_tx");
MODULE_LICENSE("GPL v2");
