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

static inline u32 RX_INTF_REG_MULTI_RST_read(void){
	return reg_read(RX_INTF_REG_MULTI_RST_ADDR);
}

static inline u32 RX_INTF_REG_MIXER_CFG_read(void){
	return reg_read(RX_INTF_REG_MIXER_CFG_ADDR);
}

static inline u32 RX_INTF_REG_IQ_SRC_SEL_read(void){
	return reg_read(RX_INTF_REG_IQ_SRC_SEL_ADDR);
}

static inline u32 RX_INTF_REG_IQ_CTRL_read(void){
	return reg_read(RX_INTF_REG_IQ_CTRL_ADDR);
}

static inline u32 RX_INTF_REG_START_TRANS_TO_PS_MODE_read(void){
	return reg_read(RX_INTF_REG_START_TRANS_TO_PS_MODE_ADDR);
}

static inline u32 RX_INTF_REG_START_TRANS_TO_PS_read(void){
	return reg_read(RX_INTF_REG_START_TRANS_TO_PS_ADDR);
}

static inline u32 RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_read(void){
	return reg_read(RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_ADDR);
}

static inline u32 RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_read(void){
	return reg_read(RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_ADDR);
}

static inline u32 RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read(void){
	return reg_read(RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR);
}

static inline u32 RX_INTF_REG_CFG_DATA_TO_ANT_read(void){
	return reg_read(RX_INTF_REG_CFG_DATA_TO_ANT_ADDR);
}

static inline u32 RX_INTF_REG_ANT_SEL_read(void){
	return reg_read(RX_INTF_REG_ANT_SEL_ADDR);
}

static inline u32 RX_INTF_REG_INTERRUPT_TEST_read(void) {
	return reg_read(RX_INTF_REG_INTERRUPT_TEST_ADDR);
}

static inline void RX_INTF_REG_MULTI_RST_write(u32 value){
	reg_write(RX_INTF_REG_MULTI_RST_ADDR, value);
}

static inline void RX_INTF_REG_M_AXIS_RST_write(u32 value){
	u32 reg_val;

	if (value==0) {
		reg_val = RX_INTF_REG_MULTI_RST_read();
		reg_val = ( reg_val&(~(1<<4)) );
		RX_INTF_REG_MULTI_RST_write(reg_val);
	} else {
		reg_val = RX_INTF_REG_MULTI_RST_read();
		reg_val = ( reg_val|(1<<4) );
		RX_INTF_REG_MULTI_RST_write(reg_val);
	}
}

static inline void RX_INTF_REG_MIXER_CFG_write(u32 value){
	reg_write(RX_INTF_REG_MIXER_CFG_ADDR, value);
}

static inline void RX_INTF_REG_IQ_SRC_SEL_write(u32 value){
	reg_write(RX_INTF_REG_IQ_SRC_SEL_ADDR, value);
}

static inline void RX_INTF_REG_IQ_CTRL_write(u32 value){
	reg_write(RX_INTF_REG_IQ_CTRL_ADDR, value);
}

static inline void RX_INTF_REG_START_TRANS_TO_PS_MODE_write(u32 value){
	reg_write(RX_INTF_REG_START_TRANS_TO_PS_MODE_ADDR, value);
}

static inline void RX_INTF_REG_START_TRANS_TO_PS_write(u32 value){
	reg_write(RX_INTF_REG_START_TRANS_TO_PS_ADDR, value);
}

static inline void RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_write(u32 value){
	reg_write(RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_ADDR, value);
}

static inline void RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write(u32 value){
	reg_write(RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_ADDR, value);
}

static inline void RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write(u32 value){
	reg_write(RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR, value);
}

static inline void RX_INTF_REG_CFG_DATA_TO_ANT_write(u32 value){
	reg_write(RX_INTF_REG_CFG_DATA_TO_ANT_ADDR, value);
}

static inline void RX_INTF_REG_ANT_SEL_write(u32 value){
	reg_write(RX_INTF_REG_ANT_SEL_ADDR, value);
}

static inline void RX_INTF_REG_INTERRUPT_TEST_write(u32 value) {
	reg_write(RX_INTF_REG_INTERRUPT_TEST_ADDR, value);
}

static inline void RX_INTF_REG_S2MM_INTR_DELAY_COUNT_write(u32 value) {
	reg_write(RX_INTF_REG_S2MM_INTR_DELAY_COUNT_ADDR, value);
}

static inline void RX_INTF_REG_TLAST_TIMEOUT_TOP_write(u32 value) {
	reg_write(RX_INTF_REG_TLAST_TIMEOUT_TOP_ADDR, value);
}

static const struct of_device_id dev_of_ids[] = {
	{ .compatible = "sdr,rx_intf", },
	{}
};
MODULE_DEVICE_TABLE(of, dev_of_ids);

static struct rx_intf_driver_api rx_intf_driver_api_inst;
//EXPORT_SYMBOL(rx_intf_driver_api_inst);
static struct rx_intf_driver_api *rx_intf_api = &rx_intf_driver_api_inst;
EXPORT_SYMBOL(rx_intf_api);

static inline u32 hw_init(enum rx_intf_mode mode, u32 num_dma_symbol_to_pl, u32 num_dma_symbol_to_ps){
	int err=0;
	u32 reg_val, mixer_cfg=0, ant_sel=0;

	printk("%s hw_init mode %d\n", rx_intf_compatible_str, mode);

	////rst wifi rx -- slv_reg11[2] is actual rx reset. slv_reg11[0] only reset axi lite of rx
	//printk("%s hw_init reset wifi rx\n", rx_intf_compatible_str);
	//rx_intf_api->RX_INTF_REG_RST_START_TO_EXT_write(0);
	//rx_intf_api->RX_INTF_REG_RST_START_TO_EXT_write(4);
	//rx_intf_api->RX_INTF_REG_RST_START_TO_EXT_write(0);

	rx_intf_api->RX_INTF_REG_TLAST_TIMEOUT_TOP_write(7000);
	//rst ddc internal module
	for (reg_val=0;reg_val<32;reg_val++)
		rx_intf_api->RX_INTF_REG_MULTI_RST_write(0xFFFFFFFF);
	rx_intf_api->RX_INTF_REG_MULTI_RST_write(0);
	rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(1); // hold M AXIS in reset status. will be released when openwifi_start

	switch(mode)
	{
		case RX_INTF_AXIS_LOOP_BACK:
			printk("%s hw_init mode RX_INTF_AXIS_LOOP_BACK\n", rx_intf_compatible_str);
			//setting the path and mode. This must be done before our dma end reset
			rx_intf_api->RX_INTF_REG_IQ_SRC_SEL_write(0x15);
			rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_write(1);
			rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_MODE_write(0x37);// endless mode to support sg DMA loop back, start 1 trans from sw trigger

			rx_intf_api->RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write(num_dma_symbol_to_pl);
			rx_intf_api->RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write(num_dma_symbol_to_ps);

			// put bb_en to constant 1
			reg_val = rx_intf_api->RX_INTF_REG_IQ_CTRL_read();
			reg_val = (reg_val|0x8);
			rx_intf_api->RX_INTF_REG_IQ_CTRL_write(reg_val);

			// connect axis slave and master directly for loopback
			rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_MODE_write(0x1037);

			// reset dma end point in our design
			reg_val = rx_intf_api->RX_INTF_REG_MULTI_RST_read();
			reg_val = (reg_val&(~0x14) );
			rx_intf_api->RX_INTF_REG_MULTI_RST_write(reg_val);
			reg_val = reg_val|(0x14);
			rx_intf_api->RX_INTF_REG_MULTI_RST_write(reg_val);
			reg_val = reg_val&(~0x14);
			rx_intf_api->RX_INTF_REG_MULTI_RST_write(reg_val);

			//start 1 trans now from our m_axis to ps dma
			rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_write(0);
			rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_write(1);
			rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_write(0);
			break;

		case RX_INTF_BW_20MHZ_AT_0MHZ_ANT0:
			printk("%s hw_init mode DDC_BW_20MHZ_AT_0MHZ\n", rx_intf_compatible_str);
			mixer_cfg = 0x300200F4;
			ant_sel=0;
			break;

		case RX_INTF_BW_20MHZ_AT_0MHZ_ANT1:
			printk("%s hw_init mode DDC_BW_20MHZ_AT_0MHZ\n", rx_intf_compatible_str);
			mixer_cfg = 0x300200F4;
			ant_sel=1;
			break;

		case RX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0:
			printk("%s hw_init mode DDC_BW_20MHZ_AT_N_10MHZ\n", rx_intf_compatible_str);
			mixer_cfg = 0x300202F6;
			ant_sel=0;
			break;

		case RX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1:
			printk("%s hw_init mode DDC_BW_20MHZ_AT_N_10MHZ\n", rx_intf_compatible_str);
			mixer_cfg = 0x300202F6;
			ant_sel=1;
			break;

		case RX_INTF_BW_20MHZ_AT_P_10MHZ_ANT0:
			printk("%s hw_init mode DDC_BW_20MHZ_AT_P_10MHZ\n", rx_intf_compatible_str);
			mixer_cfg = 0x3001F602;
			ant_sel=0;
			break;

		case RX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1:
			printk("%s hw_init mode DDC_BW_20MHZ_AT_P_10MHZ\n", rx_intf_compatible_str);
			mixer_cfg = 0x3001F602;
			ant_sel=1;
			break;

		case RX_INTF_BYPASS:
			printk("%s hw_init mode DDC_BYPASS\n", rx_intf_compatible_str);
			mixer_cfg = 0x3001F602;
			break;
		
		default:
			printk("%s hw_init mode %d is wrong!\n", rx_intf_compatible_str, mode);
			err=1;
	}

	if (mode!=RX_INTF_AXIS_LOOP_BACK) {
		rx_intf_api->RX_INTF_REG_MIXER_CFG_write(mixer_cfg);
		// 0x000202F6 for: wifi ant0: -10MHz; wifi ant1: +10MHz; zigbee 4 ch ant0: -2, -7, -12, -17MHz; zigbee 4 ch ant1: +3, +8, +13, +18MHz
		// 0x0001F602 for: wifi ant0: +10MHz; wifi ant1: -10MHz; zigbee 4 ch ant0: +3, +8, +13, +18MHz; zigbee 4 ch ant1: -2, -7, -12, -17MHz
		// 0x0001F206 for: wifi ant0: -10MHz; wifi ant1: +10MHz; zigbee 4 ch ant0: +3, +8, +13, +18MHz; zigbee 4 ch ant1: -2, -7, -12, -17MHz
		// 0x2101F602 for: wifi gain 4;   zigbee gain 2
		// 0xFE01F602 for: wifi gain 1/2; zigbee gain 1/4
		// bits definitions:
		// wifi ch selection:     ant0 bit1~0; ant1 bit 9~8; ch offset: 0-0MHz; 1-5MHz; 2-10MHz; 3-15MHz(severe distortion)
		// wifi ch +/- selection: ant0 bit2; ant1 bit 10; 0-positive; 1-negative
		// zigbee 2M mixer +/- selection:        ant0   bit3; ant1    bit 11; 0-positive; 1-negative
		// zigbee secondary mixer +/- selection: ant0 bit4~7; ant1 bit 12~15; 0-positive; 1-negative
		// zigbee ch slip offset:                ant0  bit16; ant1     bit17; 0-select ch offset 0, 5, 10, 15; 1-select ch offset 5 10 15 20
		// wifi gain: bit31~28; number of bits shifted to left in 2'complement code
		// zigb gain: bit27~24; number of bits shifted to left in 2'complement code
		// max amplitude calibration info (agc low, ddc w/o gain adj 0x0001F602): 5GHz, max amplitude 1.26e4. According to simulation, schr shrink 1bit should be enough
		
		rx_intf_api->RX_INTF_REG_MULTI_RST_write(0);
		rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(1); // hold M AXIS in reset status. will be released when openwifi_start
		
		//rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_write(0x000);
		rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_write(0x100);
		//0x000-normal; 0x100-sig and fcs valid are controled by bit4 and bit0;
		//0x111-sig and fcs high; 0x110-sig high fcs low; 0x101-sig low fcs high; 0x100-sig and fcs low

		rx_intf_api->RX_INTF_REG_IQ_SRC_SEL_write(0);
		// 0-bw20-ch0; 1-bw2-ch0;  2-bw2-ch2;  3-bw2-ch4;  4-bw2-ch6;  5-s_axis-ch0
		// 8-bw20-ch1; 9-bw2-ch1; 10-bw2-ch3; 11-bw2-ch5; 12-bw2-ch7; 13-s_axis-ch1

		//rx_intf_api->RX_INTF_REG_S2MM_INTR_DELAY_COUNT_write(1000|0x80000000); //0x80000000 to enable tsft and rssi gpio test magic value
		//rx_intf_api->RX_INTF_REG_S2MM_INTR_DELAY_COUNT_write(200*10); //0x80000000 to enable tsft and rssi gpio test magic value
		rx_intf_api->RX_INTF_REG_S2MM_INTR_DELAY_COUNT_write(30*200); // delayed interrupt
		
		rx_intf_api->RX_INTF_REG_IQ_CTRL_write(0);
		rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_MODE_write(0x10025); //now bit 5 should be 1 to let pl_to_m_axis_intf decide num_dma_symbol_to_ps automatically
		//rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_MODE_write(0x00025); //bit16 enable_m_axis_auto_rst
		//bit2-0: source of M AXIS transfer trigger
		//		 -0 fcs_valid_from_acc
		//		 -1 sig_valid_from_acc
		//		 -2 sig_invalid_from_acc
		//		 -3 start_1trans_s_axis_tlast_trigger
		//		 -4 start_1trans_s_axis_tready_trigger
		//		 -5 internal state machine together with bit5 1. By parsing signal field, num_dma_symbol_to_ps can be decided automatically
		//		 -6 start_1trans_monitor_dma_to_ps_start_trigger
		//		 -7 start_1trans_ext_trigger
		//bit3:  1-fcs valid and invalid both connected; 0-only fcs valid connected (fcs_invalid_mode)
		//bit4:  1-num_dma_symbol_to_pl from monitor; 0-num_dma_symbol_to_pl from slv_reg8
		//bit5:  1-num_dma_symbol_to_ps from monitor; 0-num_dma_symbol_to_ps from slv_reg9
		//bit6:  1-pl_to_m_axis_intf will try to send both ht and non-ht; 0-only send non-ht
		//bit8:  1-endless S AXIS; 0-normal
		//bit9:  1-endless M AXIS; 0-normal
		//bit12: 1-direct loop back; 0-normal
		//bit16: 1-auto m_axis rst (sig_valid_from_acc|sig_invalid_from_acc|ht_sig_valid|ht_sig_invalid|ht_unsupported); 0-normal
		//bit24: 1-disable m_axis fifo_rst_by_fcs_invalid; 0-enable
		//bit29,28: sig_valid_mode. 0- non-ht sig valid; 1- ht sig valid other- both
		rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_write(0);
		rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_write(0);
		// 0-wifi_rx packet out; 1-loopback from input of wifi_rx
		
		rx_intf_api->RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write(num_dma_symbol_to_pl);
		rx_intf_api->RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write(num_dma_symbol_to_ps);
		rx_intf_api->RX_INTF_REG_CFG_DATA_TO_ANT_write(1<<8);
		rx_intf_api->RX_INTF_REG_ANT_SEL_write(ant_sel);

		rx_intf_api->RX_INTF_REG_MULTI_RST_write(0x14);//rst m/s axis
		rx_intf_api->RX_INTF_REG_MULTI_RST_write(0);
		rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(1); // hold M AXIS in reset status. will be released when openwifi_start
	}

	if (mode==RX_INTF_BYPASS) {
		rx_intf_api->RX_INTF_REG_CFG_DATA_TO_ANT_write(0x10); //bit4 bypass enable
	}

	printk("%s hw_init err %d\n", rx_intf_compatible_str, err);
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
			printk("%s dev_probe match!\n", rx_intf_compatible_str);
			err = 0;
		}
	}

	if (err)
		return err;

	rx_intf_api->hw_init=hw_init;

	rx_intf_api->reg_read=reg_read;
	rx_intf_api->reg_write=reg_write;

	rx_intf_api->RX_INTF_REG_MULTI_RST_read=RX_INTF_REG_MULTI_RST_read;
	rx_intf_api->RX_INTF_REG_MIXER_CFG_read=RX_INTF_REG_MIXER_CFG_read;
	rx_intf_api->RX_INTF_REG_IQ_SRC_SEL_read=RX_INTF_REG_IQ_SRC_SEL_read;
	rx_intf_api->RX_INTF_REG_IQ_CTRL_read=RX_INTF_REG_IQ_CTRL_read;
	rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_MODE_read=RX_INTF_REG_START_TRANS_TO_PS_MODE_read;
	rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_read=RX_INTF_REG_START_TRANS_TO_PS_read;
	rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_read=RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_read;
	rx_intf_api->RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_read=RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_read;
	rx_intf_api->RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read=RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read;
	rx_intf_api->RX_INTF_REG_CFG_DATA_TO_ANT_read=RX_INTF_REG_CFG_DATA_TO_ANT_read;
	rx_intf_api->RX_INTF_REG_ANT_SEL_read=RX_INTF_REG_ANT_SEL_read;
	rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_read=RX_INTF_REG_INTERRUPT_TEST_read;

	rx_intf_api->RX_INTF_REG_MULTI_RST_write=RX_INTF_REG_MULTI_RST_write;
	rx_intf_api->RX_INTF_REG_M_AXIS_RST_write=RX_INTF_REG_M_AXIS_RST_write;
	rx_intf_api->RX_INTF_REG_MIXER_CFG_write=RX_INTF_REG_MIXER_CFG_write;
	rx_intf_api->RX_INTF_REG_IQ_SRC_SEL_write=RX_INTF_REG_IQ_SRC_SEL_write;
	rx_intf_api->RX_INTF_REG_IQ_CTRL_write=RX_INTF_REG_IQ_CTRL_write;
	rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_MODE_write=RX_INTF_REG_START_TRANS_TO_PS_MODE_write;
	rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_write=RX_INTF_REG_START_TRANS_TO_PS_write;
	rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_write=RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_write;
	rx_intf_api->RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write=RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write;
	rx_intf_api->RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write=RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write;
	rx_intf_api->RX_INTF_REG_CFG_DATA_TO_ANT_write=RX_INTF_REG_CFG_DATA_TO_ANT_write;
	rx_intf_api->RX_INTF_REG_ANT_SEL_write=RX_INTF_REG_ANT_SEL_write;
	rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_write=RX_INTF_REG_INTERRUPT_TEST_write;

	rx_intf_api->RX_INTF_REG_S2MM_INTR_DELAY_COUNT_write=RX_INTF_REG_S2MM_INTR_DELAY_COUNT_write;
	rx_intf_api->RX_INTF_REG_TLAST_TIMEOUT_TOP_write=RX_INTF_REG_TLAST_TIMEOUT_TOP_write;

	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base_addr = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(base_addr))
		return PTR_ERR(base_addr);

	rx_intf_api->io_start = io->start;
	rx_intf_api->base_addr = (u32)base_addr;

	printk("%s dev_probe io start 0x%08x end 0x%08x name %s flags 0x%08x desc 0x%08x\n", rx_intf_compatible_str,io->start,io->end,io->name,(u32)io->flags,(u32)io->desc);
	printk("%s dev_probe base_addr 0x%08x\n", rx_intf_compatible_str,(u32)base_addr);
	printk("%s dev_probe rx_intf_driver_api_inst 0x%08x\n", rx_intf_compatible_str, (u32)(&rx_intf_driver_api_inst) );
	printk("%s dev_probe             rx_intf_api 0x%08x\n", rx_intf_compatible_str, (u32)rx_intf_api);

	printk("%s dev_probe succeed!\n", rx_intf_compatible_str);

	//err = hw_init(DDC_CURRENT_CH_OFFSET_CFG,8,8);
	err = hw_init(RX_INTF_BW_20MHZ_AT_0MHZ_ANT0,8,8);

	return err;
}

static int dev_remove(struct platform_device *pdev)
{
	printk("\n");

	printk("%s dev_remove base_addr 0x%08x\n", rx_intf_compatible_str, (u32)base_addr);
	printk("%s dev_remove rx_intf_driver_api_inst 0x%08x\n", rx_intf_compatible_str, (u32)(&rx_intf_driver_api_inst) );
	printk("%s dev_remove             rx_intf_api 0x%08x\n", rx_intf_compatible_str, (u32)rx_intf_api);

	printk("%s dev_remove succeed!\n", rx_intf_compatible_str);
	return 0;
}

static struct platform_driver dev_driver = {
	.driver = {
		.name = "sdr,rx_intf",
		.owner = THIS_MODULE,
		.of_match_table = dev_of_ids,
	},
	.probe = dev_probe,
	.remove = dev_remove,
};

module_platform_driver(dev_driver);

MODULE_AUTHOR("Xianjun Jiao");
MODULE_DESCRIPTION("sdr,rx_intf");
MODULE_LICENSE("GPL v2");
