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

static inline u32 TX_INTF_REG_MULTI_RST_read(void){
	return reg_read(TX_INTF_REG_MULTI_RST_ADDR);
}

static inline u32 TX_INTF_REG_MIXER_CFG_read(void){
	return reg_read(TX_INTF_REG_MIXER_CFG_ADDR);
}

static inline u32 TX_INTF_REG_WIFI_TX_MODE_read(void){
	return reg_read(TX_INTF_REG_WIFI_TX_MODE_ADDR);
}

static inline u32 TX_INTF_REG_IQ_SRC_SEL_read(void){
	return reg_read(TX_INTF_REG_IQ_SRC_SEL_ADDR);
}

static inline u32 TX_INTF_REG_CTS_TOSELF_CONFIG_read(void){
	return reg_read(TX_INTF_REG_CTS_TOSELF_CONFIG_ADDR);
}

static inline u32 TX_INTF_REG_START_TRANS_TO_PS_MODE_read(void){
	return reg_read(TX_INTF_REG_START_TRANS_TO_PS_MODE_ADDR);
}

static inline u32 TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_read(void){
	return reg_read(TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_ADDR);
}

static inline u32 TX_INTF_REG_MISC_SEL_read(void){
	return reg_read(TX_INTF_REG_MISC_SEL_ADDR);
}

static inline u32 TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_read(void){
	return reg_read(TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_ADDR);
}

static inline u32 TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read(void){
	return reg_read(TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR);
}

static inline u32 TX_INTF_REG_CFG_DATA_TO_ANT_read(void){
	return reg_read(TX_INTF_REG_CFG_DATA_TO_ANT_ADDR);
}

static inline u32 TX_INTF_REG_INTERRUPT_SEL_read(void){
	return reg_read(TX_INTF_REG_INTERRUPT_SEL_ADDR);
}

static inline u32 TX_INTF_REG_BB_GAIN_read(void){
	return reg_read(TX_INTF_REG_BB_GAIN_ADDR);
}

static inline u32 TX_INTF_REG_ANT_SEL_read(void){
	return reg_read(TX_INTF_REG_ANT_SEL_ADDR);
}

static inline u32 TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_read(void){
	return reg_read(TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_ADDR);
}

static inline u32 TX_INTF_REG_PKT_INFO_read(void){
	return reg_read(TX_INTF_REG_PKT_INFO_ADDR);
}

static inline u32 TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read(void){
	return reg_read(TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_ADDR);
}

//--------------------------------------------------------

static inline void TX_INTF_REG_MULTI_RST_write(u32 value){
	reg_write(TX_INTF_REG_MULTI_RST_ADDR, value);
}

static inline void TX_INTF_REG_MIXER_CFG_write(u32 value){
	reg_write(TX_INTF_REG_MIXER_CFG_ADDR, value);
}

static inline void TX_INTF_REG_WIFI_TX_MODE_write(u32 value){
	reg_write(TX_INTF_REG_WIFI_TX_MODE_ADDR, value);
}

static inline void TX_INTF_REG_IQ_SRC_SEL_write(u32 value){
	reg_write(TX_INTF_REG_IQ_SRC_SEL_ADDR, value);
}

static inline void TX_INTF_REG_CTS_TOSELF_CONFIG_write(u32 value){
	reg_write(TX_INTF_REG_CTS_TOSELF_CONFIG_ADDR, value);
}

static inline void TX_INTF_REG_START_TRANS_TO_PS_MODE_write(u32 value){
	reg_write(TX_INTF_REG_START_TRANS_TO_PS_MODE_ADDR, value);
}

static inline void TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write(u32 value){
	reg_write(TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_ADDR, value);
}

static inline void TX_INTF_REG_MISC_SEL_write(u32 value){
	reg_write(TX_INTF_REG_MISC_SEL_ADDR, value);
}

static inline void TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write(u32 value){
	reg_write(TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_ADDR, value);
}

static inline void TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write(u32 value){
	reg_write(TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR, value);
}

static inline void TX_INTF_REG_CFG_DATA_TO_ANT_write(u32 value){
	reg_write(TX_INTF_REG_CFG_DATA_TO_ANT_ADDR, value);
}

static inline void TX_INTF_REG_INTERRUPT_SEL_write(u32 value){
	reg_write(TX_INTF_REG_INTERRUPT_SEL_ADDR, value);
}

static inline void TX_INTF_REG_BB_GAIN_write(u32 value){
	reg_write(TX_INTF_REG_BB_GAIN_ADDR, value);
}

static inline void TX_INTF_REG_ANT_SEL_write(u32 value){
	reg_write(TX_INTF_REG_ANT_SEL_ADDR, value);
}

static inline void TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_write(u32 value){
	reg_write(TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_ADDR, value);
}

static inline void TX_INTF_REG_PKT_INFO_write(u32 value){
	reg_write(TX_INTF_REG_PKT_INFO_ADDR,value);
}

static const struct of_device_id dev_of_ids[] = {
	{ .compatible = "sdr,tx_intf", },
	{}
};
MODULE_DEVICE_TABLE(of, dev_of_ids);

static struct tx_intf_driver_api tx_intf_driver_api_inst;
static struct tx_intf_driver_api *tx_intf_api = &tx_intf_driver_api_inst;
EXPORT_SYMBOL(tx_intf_api);

static inline u32 hw_init(enum tx_intf_mode mode, u32 num_dma_symbol_to_pl, u32 num_dma_symbol_to_ps){
	int err=0;
	u32 reg_val, mixer_cfg=0, duc_input_ch_sel = 0, ant_sel=0;

	printk("%s hw_init mode %d\n", tx_intf_compatible_str, mode);

	//rst duc internal module
	for (reg_val=0;reg_val<32;reg_val++)
		tx_intf_api->TX_INTF_REG_MULTI_RST_write(0xFFFFFFFF);
	tx_intf_api->TX_INTF_REG_MULTI_RST_write(0);

	switch(mode)
	{
		case TX_INTF_AXIS_LOOP_BACK:
			tx_intf_api->TX_INTF_REG_MISC_SEL_write(0<<1);// bit1: 0-connect dac to ADI dma; 1-connect dac to our intf
			printk("%s hw_init mode TX_INTF_AXIS_LOOP_BACK\n", tx_intf_compatible_str);
			break;

		case TX_INTF_BW_20MHZ_AT_0MHZ_ANT0:
			printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_0MHZ_ANT0\n", tx_intf_compatible_str);
			mixer_cfg = 0x2001F400;
			duc_input_ch_sel = 0;
			ant_sel=1;
			break;

		case TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0:
			printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0\n", tx_intf_compatible_str);
			mixer_cfg = 0x2001F602;
			duc_input_ch_sel = 0;
			ant_sel=1;
			break;

		case TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT0:
			printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT0\n", tx_intf_compatible_str);
			mixer_cfg = 0x200202F6;
			duc_input_ch_sel = 0;
			ant_sel=1;
			break;

		case TX_INTF_BW_20MHZ_AT_0MHZ_ANT1:
			printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_0MHZ_ANT1\n", tx_intf_compatible_str);
			mixer_cfg = 0x2001F400;
			duc_input_ch_sel = 0;
			ant_sel=2;
			break;

		case TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1:
			printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1\n", tx_intf_compatible_str);
			mixer_cfg = 0x2001F602;
			duc_input_ch_sel = 0;
			ant_sel=2;
			break;

		case TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1:
			printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1\n", tx_intf_compatible_str);
			mixer_cfg = 0x200202F6;
			duc_input_ch_sel = 0;
			ant_sel=2;
			break;

		case TX_INTF_BYPASS:
			printk("%s hw_init mode TX_INTF_BYPASS\n", tx_intf_compatible_str);
			mixer_cfg = 0x200202F6;
			duc_input_ch_sel = 0;
			ant_sel=2;
			break;
		
		default:
			printk("%s hw_init mode %d is wrong!\n", tx_intf_compatible_str, mode);
			err=1;
	}

	if (mode!=TX_INTF_AXIS_LOOP_BACK) {
		tx_intf_api->TX_INTF_REG_MISC_SEL_write(1<<1);// bit1: 0-connect dac to ADI dma; 1-connect dac to our intf

		tx_intf_api->TX_INTF_REG_MIXER_CFG_write(mixer_cfg);
		tx_intf_api->TX_INTF_REG_MULTI_RST_write(0);
		tx_intf_api->TX_INTF_REG_IQ_SRC_SEL_write(duc_input_ch_sel);
		tx_intf_api->TX_INTF_REG_START_TRANS_TO_PS_MODE_write(2);
		tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write( ((16*200)<<16)|(10*200) );//high 16bit 5GHz; low 16 bit 2.4GHz

		tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write(num_dma_symbol_to_pl);
		tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write(num_dma_symbol_to_ps);
		tx_intf_api->TX_INTF_REG_CFG_DATA_TO_ANT_write(0);
		tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x40); //.src_sel0(slv_reg14[2:0]), .src_sel1(slv_reg14[6:4]), 0-s00_axis_tlast,1-ap_start,2-tx_start_from_acc,3-tx_end_from_acc,4-xpu signal
		tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x30040); //disable interrupt
		tx_intf_api->TX_INTF_REG_BB_GAIN_write(237);
		tx_intf_api->TX_INTF_REG_ANT_SEL_write(ant_sel);
		tx_intf_api->TX_INTF_REG_WIFI_TX_MODE_write((1<<3)|(2<<4));
		tx_intf_api->TX_INTF_REG_MULTI_RST_write(0x434);
		tx_intf_api->TX_INTF_REG_MULTI_RST_write(0);
	}

	if (mode == TX_INTF_BYPASS) {
		tx_intf_api->TX_INTF_REG_CFG_DATA_TO_ANT_write(0x100); //slv_reg10[8]
	}

	printk("%s hw_init err %d\n", tx_intf_compatible_str, err);
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
			printk("%s dev_probe match!\n", tx_intf_compatible_str);
			err = 0;
		}
	}

	if (err)
		return err;

	tx_intf_api->hw_init=hw_init;

	tx_intf_api->reg_read=reg_read;
	tx_intf_api->reg_write=reg_write;

	tx_intf_api->TX_INTF_REG_MULTI_RST_read=TX_INTF_REG_MULTI_RST_read;
	tx_intf_api->TX_INTF_REG_MIXER_CFG_read=TX_INTF_REG_MIXER_CFG_read;
	tx_intf_api->TX_INTF_REG_WIFI_TX_MODE_read=TX_INTF_REG_WIFI_TX_MODE_read;
	tx_intf_api->TX_INTF_REG_IQ_SRC_SEL_read=TX_INTF_REG_IQ_SRC_SEL_read;
	tx_intf_api->TX_INTF_REG_CTS_TOSELF_CONFIG_read=TX_INTF_REG_CTS_TOSELF_CONFIG_read;
	tx_intf_api->TX_INTF_REG_START_TRANS_TO_PS_MODE_read=TX_INTF_REG_START_TRANS_TO_PS_MODE_read;
	tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_read=TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_read;
	tx_intf_api->TX_INTF_REG_MISC_SEL_read=TX_INTF_REG_MISC_SEL_read;
	tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_read=TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_read;
	tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read=TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read;
	tx_intf_api->TX_INTF_REG_CFG_DATA_TO_ANT_read=TX_INTF_REG_CFG_DATA_TO_ANT_read;
	tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_read=TX_INTF_REG_INTERRUPT_SEL_read;
	tx_intf_api->TX_INTF_REG_BB_GAIN_read=TX_INTF_REG_BB_GAIN_read;
	tx_intf_api->TX_INTF_REG_ANT_SEL_read=TX_INTF_REG_ANT_SEL_read;
	tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_read=TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_read;
	tx_intf_api->TX_INTF_REG_PKT_INFO_read=TX_INTF_REG_PKT_INFO_read;
	tx_intf_api->TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read=TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read;

	tx_intf_api->TX_INTF_REG_MULTI_RST_write=TX_INTF_REG_MULTI_RST_write;
	tx_intf_api->TX_INTF_REG_MIXER_CFG_write=TX_INTF_REG_MIXER_CFG_write;
	tx_intf_api->TX_INTF_REG_WIFI_TX_MODE_write=TX_INTF_REG_WIFI_TX_MODE_write;
	tx_intf_api->TX_INTF_REG_IQ_SRC_SEL_write=TX_INTF_REG_IQ_SRC_SEL_write;
	tx_intf_api->TX_INTF_REG_CTS_TOSELF_CONFIG_write=TX_INTF_REG_CTS_TOSELF_CONFIG_write;
	tx_intf_api->TX_INTF_REG_START_TRANS_TO_PS_MODE_write=TX_INTF_REG_START_TRANS_TO_PS_MODE_write;
	tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write=TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write;
	tx_intf_api->TX_INTF_REG_MISC_SEL_write=TX_INTF_REG_MISC_SEL_write;
	tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write=TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write;
	tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write=TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write;
	tx_intf_api->TX_INTF_REG_CFG_DATA_TO_ANT_write=TX_INTF_REG_CFG_DATA_TO_ANT_write;
	tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write=TX_INTF_REG_INTERRUPT_SEL_write;
	tx_intf_api->TX_INTF_REG_BB_GAIN_write=TX_INTF_REG_BB_GAIN_write;
	tx_intf_api->TX_INTF_REG_ANT_SEL_write=TX_INTF_REG_ANT_SEL_write;
	tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_write=TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_write;
	tx_intf_api->TX_INTF_REG_PKT_INFO_write=TX_INTF_REG_PKT_INFO_write;

	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base_addr = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(base_addr))
		return PTR_ERR(base_addr);

	printk("%s dev_probe io start 0x%08x end 0x%08x name %s flags 0x%08x desc 0x%08x\n", tx_intf_compatible_str,io->start,io->end,io->name,(u32)io->flags,(u32)io->desc);
	printk("%s dev_probe base_addr 0x%08x\n", tx_intf_compatible_str,(u32)base_addr);
	printk("%s dev_probe tx_intf_driver_api_inst 0x%08x\n", tx_intf_compatible_str, (u32)(&tx_intf_driver_api_inst) );
	printk("%s dev_probe             tx_intf_api 0x%08x\n", tx_intf_compatible_str, (u32)tx_intf_api);

	printk("%s dev_probe succeed!\n", tx_intf_compatible_str);

	//err = hw_init(TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1, 8, 8);
	//err = hw_init(TX_INTF_BYPASS, 8, 8);
	err = hw_init(TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1, 8, 8); // make sure dac is connected to original ad9361 dma

	return err;
}

static int dev_remove(struct platform_device *pdev)
{
	printk("\n");

	printk("%s dev_remove base_addr 0x%08x\n", tx_intf_compatible_str,(u32)base_addr);
	printk("%s dev_remove tx_intf_driver_api_inst 0x%08x\n", tx_intf_compatible_str, (u32)(&tx_intf_driver_api_inst) );
	printk("%s dev_remove             tx_intf_api 0x%08x\n", tx_intf_compatible_str, (u32)tx_intf_api);

	printk("%s dev_remove succeed!\n", tx_intf_compatible_str);
	return 0;
}

static struct platform_driver dev_driver = {
	.driver = {
		.name = "sdr,tx_intf",
		.owner = THIS_MODULE,
		.of_match_table = dev_of_ids,
	},
	.probe = dev_probe,
	.remove = dev_remove,
};

module_platform_driver(dev_driver);

MODULE_AUTHOR("Xianjun Jiao");
MODULE_DESCRIPTION("sdr,tx_intf");
MODULE_LICENSE("GPL v2");
