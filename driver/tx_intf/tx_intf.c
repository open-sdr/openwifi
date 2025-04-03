/*
 * axi lite register access driver
 * Author: Xianjun Jiao, Michael Mehari, Wei Liu
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

static inline u32 TX_INTF_REG_ARBITRARY_IQ_read(void){
  return reg_read(TX_INTF_REG_ARBITRARY_IQ_ADDR);
}

static inline u32 TX_INTF_REG_WIFI_TX_MODE_read(void){
  return reg_read(TX_INTF_REG_WIFI_TX_MODE_ADDR);
}

static inline u32 TX_INTF_REG_CTS_TOSELF_CONFIG_read(void){
  return reg_read(TX_INTF_REG_CTS_TOSELF_CONFIG_ADDR);
}

static inline u32 TX_INTF_REG_CSI_FUZZER_read(void){
  return reg_read(TX_INTF_REG_CSI_FUZZER_ADDR);
}

static inline u32 TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_read(void){
  return reg_read(TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_ADDR);
}

static inline u32 TX_INTF_REG_ARBITRARY_IQ_CTL_read(void){
  return reg_read(TX_INTF_REG_ARBITRARY_IQ_CTL_ADDR);
}

static inline u32 TX_INTF_REG_TX_CONFIG_read(void){
  return reg_read(TX_INTF_REG_TX_CONFIG_ADDR);
}

static inline u32 TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read(void){
  return reg_read(TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR);
}

static inline u32 TX_INTF_REG_CFG_DATA_TO_ANT_read(void){
  return reg_read(TX_INTF_REG_CFG_DATA_TO_ANT_ADDR);
}

static inline u32 TX_INTF_REG_S_AXIS_FIFO_TH_read(void){
  return reg_read(TX_INTF_REG_S_AXIS_FIFO_TH_ADDR);
}

static inline u32 TX_INTF_REG_TX_HOLD_THRESHOLD_read(void){
  return reg_read(TX_INTF_REG_TX_HOLD_THRESHOLD_ADDR);
}

static inline u32 TX_INTF_REG_INTERRUPT_SEL_read(void){
  return reg_read(TX_INTF_REG_INTERRUPT_SEL_ADDR);
}

static inline u32 TX_INTF_REG_AMPDU_ACTION_CONFIG_read(void){
  return reg_read(TX_INTF_REG_AMPDU_ACTION_CONFIG_ADDR);
}

static inline u32 TX_INTF_REG_BB_GAIN_read(void){
  return reg_read(TX_INTF_REG_BB_GAIN_ADDR);
}

static inline u32 TX_INTF_REG_ANT_SEL_read(void){
  return reg_read(TX_INTF_REG_ANT_SEL_ADDR);
}

static inline u32 TX_INTF_REG_PHY_HDR_CONFIG_read(void){
  return reg_read(TX_INTF_REG_PHY_HDR_CONFIG_ADDR);
}

static inline u32 TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read(void){
  return reg_read(TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_ADDR);
}

static inline u32 TX_INTF_REG_PKT_INFO1_read(void){
  return reg_read(TX_INTF_REG_PKT_INFO1_ADDR);
}

static inline u32 TX_INTF_REG_PKT_INFO2_read(void){
  return reg_read(TX_INTF_REG_PKT_INFO2_ADDR);
}

static inline u32 TX_INTF_REG_PKT_INFO3_read(void){
  return reg_read(TX_INTF_REG_PKT_INFO3_ADDR);
}

static inline u32 TX_INTF_REG_PKT_INFO4_read(void){
  return reg_read(TX_INTF_REG_PKT_INFO4_ADDR);
}

static inline u32 TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read(void){
  return reg_read(TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_ADDR);
}

//--------------------------------------------------------

static inline void TX_INTF_REG_MULTI_RST_write(u32 value){
  reg_write(TX_INTF_REG_MULTI_RST_ADDR, value);
}

static inline void TX_INTF_REG_ARBITRARY_IQ_write(u32 value){
  reg_write(TX_INTF_REG_ARBITRARY_IQ_ADDR, value);
}

static inline void TX_INTF_REG_WIFI_TX_MODE_write(u32 value){
  reg_write(TX_INTF_REG_WIFI_TX_MODE_ADDR, value);
}

static inline void TX_INTF_REG_CTS_TOSELF_CONFIG_write(u32 value){
  reg_write(TX_INTF_REG_CTS_TOSELF_CONFIG_ADDR, value);
}

static inline void TX_INTF_REG_CSI_FUZZER_write(u32 value){
  reg_write(TX_INTF_REG_CSI_FUZZER_ADDR, value);
}

static inline void TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write(u32 value){
  reg_write(TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_ADDR, value);
}

static inline void TX_INTF_REG_ARBITRARY_IQ_CTL_write(u32 value){
  reg_write(TX_INTF_REG_ARBITRARY_IQ_CTL_ADDR, value);
}

static inline void TX_INTF_REG_TX_CONFIG_write(u32 value){
  reg_write(TX_INTF_REG_TX_CONFIG_ADDR, value);
}

static inline void TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write(u32 value){
  reg_write(TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR, value);
}

static inline void TX_INTF_REG_CFG_DATA_TO_ANT_write(u32 value){
  reg_write(TX_INTF_REG_CFG_DATA_TO_ANT_ADDR, value);
}

static inline void TX_INTF_REG_S_AXIS_FIFO_TH_write(u32 value){
  reg_write(TX_INTF_REG_S_AXIS_FIFO_TH_ADDR, value);
}

static inline void TX_INTF_REG_TX_HOLD_THRESHOLD_write(u32 value){
  reg_write(TX_INTF_REG_TX_HOLD_THRESHOLD_ADDR, value);
}

static inline void TX_INTF_REG_INTERRUPT_SEL_write(u32 value){
  reg_write(TX_INTF_REG_INTERRUPT_SEL_ADDR, value);
}

static inline void TX_INTF_REG_AMPDU_ACTION_CONFIG_write(u32 value){
  reg_write(TX_INTF_REG_AMPDU_ACTION_CONFIG_ADDR, value);
}

static inline void TX_INTF_REG_BB_GAIN_write(u32 value){
  reg_write(TX_INTF_REG_BB_GAIN_ADDR, value);
}

static inline void TX_INTF_REG_ANT_SEL_write(u32 value){
  reg_write(TX_INTF_REG_ANT_SEL_ADDR, value);
}

static inline void TX_INTF_REG_PHY_HDR_CONFIG_write(u32 value){
  reg_write(TX_INTF_REG_PHY_HDR_CONFIG_ADDR, value);
}

static inline void TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_write(u32 value){
  reg_write(TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_ADDR, value);
}

static inline void TX_INTF_REG_PKT_INFO1_write(u32 value){
  reg_write(TX_INTF_REG_PKT_INFO1_ADDR,value);
}

static inline void TX_INTF_REG_PKT_INFO2_write(u32 value){
  reg_write(TX_INTF_REG_PKT_INFO2_ADDR,value);
}

static inline void TX_INTF_REG_PKT_INFO3_write(u32 value){
  reg_write(TX_INTF_REG_PKT_INFO3_ADDR,value);
}

static inline void TX_INTF_REG_PKT_INFO4_write(u32 value){
  reg_write(TX_INTF_REG_PKT_INFO4_ADDR,value);
}

static const struct of_device_id dev_of_ids[] = {
  { .compatible = "sdr,tx_intf", },
  {}
};
MODULE_DEVICE_TABLE(of, dev_of_ids);

static struct tx_intf_driver_api tx_intf_driver_api_inst;
struct tx_intf_driver_api *tx_intf_api = &tx_intf_driver_api_inst;
EXPORT_SYMBOL(tx_intf_api);

static inline u32 hw_init(enum tx_intf_mode mode, u32 tx_config, u32 num_dma_symbol_to_ps, enum openwifi_fpga_type fpga_type){
  int err=0, i;
  u32 mixer_cfg=0, ant_sel=0;

  printk("%s hw_init mode %d\n", tx_intf_compatible_str, mode);

  //rst
  for (i=0;i<8;i++)
    tx_intf_api->TX_INTF_REG_MULTI_RST_write(0);
  for (i=0;i<32;i++)
    tx_intf_api->TX_INTF_REG_MULTI_RST_write(0xFFFFFFFF);
  for (i=0;i<8;i++)
    tx_intf_api->TX_INTF_REG_MULTI_RST_write(0);

  if(fpga_type == LARGE_FPGA)  // LARGE FPGA: MAX_NUM_DMA_SYMBOL = 8192
    // tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_TH_write(8192-(210*5)); // threshold is for room to hold the last 4 packets from 4 queue before stop
    tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_TH_write(8192-(210*2));
  else if(fpga_type == SMALL_FPGA)  // SMALL FPGA: MAX_NUM_DMA_SYMBOL = 4096
    // tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_TH_write(4096-(210*5)); // threshold is for room to hold the last 4 packets from 4 queue before stop
    tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_TH_write(4096-(210*2));

  switch(mode)
  {
    case TX_INTF_AXIS_LOOP_BACK:
      printk("%s hw_init mode TX_INTF_AXIS_LOOP_BACK\n", tx_intf_compatible_str);
      break;

    case TX_INTF_BW_20MHZ_AT_0MHZ_ANT0:
      printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_0MHZ_ANT0\n", tx_intf_compatible_str);
      mixer_cfg = 0x2001F400;
      ant_sel=1;
      break;

    case TX_INTF_BW_20MHZ_AT_0MHZ_ANT_BOTH:
      printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_0MHZ_ANT_BOTH\n", tx_intf_compatible_str);
      mixer_cfg = 0x2001F400;
      ant_sel=0x11;
      break;

    case TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0:
      printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0\n", tx_intf_compatible_str);
      mixer_cfg = 0x2001F602;
      ant_sel=1;
      break;

    case TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT0:
      printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT0\n", tx_intf_compatible_str);
      mixer_cfg = 0x200202F6;
      ant_sel=1;
      break;

    case TX_INTF_BW_20MHZ_AT_0MHZ_ANT1:
      printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_0MHZ_ANT1\n", tx_intf_compatible_str);
      mixer_cfg = 0x2001F400;
      ant_sel=2;
      break;

    case TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1:
      printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1\n", tx_intf_compatible_str);
      mixer_cfg = 0x2001F602;
      ant_sel=2;
      break;

    case TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1:
      printk("%s hw_init mode TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1\n", tx_intf_compatible_str);
      mixer_cfg = 0x200202F6;
      ant_sel=2;
      break;

    case TX_INTF_BYPASS:
      printk("%s hw_init mode TX_INTF_BYPASS\n", tx_intf_compatible_str);
      mixer_cfg = 0x200202F6;
      ant_sel=2;
      break;
    
    default:
      printk("%s hw_init mode %d is wrong!\n", tx_intf_compatible_str, mode);
      err=1;
  }

  if (mode!=TX_INTF_AXIS_LOOP_BACK) {
    tx_intf_api->TX_INTF_REG_MULTI_RST_write(0);
    tx_intf_api->TX_INTF_REG_CSI_FUZZER_write(0);
    tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write( ((16*10)<<16)|(16*10) );//high 16bit 5GHz; low 16 bit 2.4GHz. counter speed 10MHz is assumed
  
    // Remove TX_INTF_REG_TX_CONFIG_write to avoid hw_init call in openwifi_start causing inconsistency
    // tx_intf_api->TX_INTF_REG_TX_CONFIG_write(tx_config);

    tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write(num_dma_symbol_to_ps);
    tx_intf_api->TX_INTF_REG_CFG_DATA_TO_ANT_write(0);
    tx_intf_api->TX_INTF_REG_TX_HOLD_THRESHOLD_write(420);
    tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x4); //.src_sel(slv_reg14[2:0]), 0-s00_axis_tlast,1-ap_start,2-tx_start_from_acc,3-tx_end_from_acc,4-tx_try_complete from xpu
    tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x30004); //disable interrupt

    // tx_intf_api->TX_INTF_REG_BB_GAIN_write(100); // value for old design with DUC (FIR + MIXER) -- obsolete due to DUC removal
    // New test on new design (unified RF BB clock; No DUC)
    // 5220MHz bb_gain power   EVM
        //         400     -6dBm   -34/35
        //         350     -7.2dBm -34/35/36
        //         300     -8.5dBm -35/36/37 EVM

        // 2437MHz bb_gain power    EVM
        //         400     -3.2dBm -36/37
        //         350     -4.4dBm -37/38/39
        //         300     -5.7dBm -39/40
        //         less    less    -40/41/42!

    // According to above and more detailed test:
    // Need to be 290. Otherwise some ofdm symbol's EVM jump high, when there are lots of ofdm symbols in one WiFi packet

    // 2022-03-04 detailed test result:
    // bb_gain 290 work for 11a/g all mcs
    // bb_gain 290 work for 11n mcs 1~7 (aggr and non aggr)
    // bb_gain 290 destroy  11n mcs 0 long (MTU 1500) tx pkt due to high PAPR (Peak to Average Power Ratio)!
    // bb_gain 250 work for 11n mcs 0
    // So, a conservative bb_gain 250 should be used
    tx_intf_api->TX_INTF_REG_BB_GAIN_write(250);

    // Remove TX_INTF_REG_ANT_SEL_write to avoid hw_init call in openwifi_start causing inconsistency
    // tx_intf_api->TX_INTF_REG_ANT_SEL_write(ant_sel);

    tx_intf_api->TX_INTF_REG_WIFI_TX_MODE_write((1<<3)|(2<<4));
    tx_intf_api->TX_INTF_REG_MULTI_RST_write(0x434);
    tx_intf_api->TX_INTF_REG_MULTI_RST_write(0);
  }

  // if (mode == TX_INTF_BYPASS) {
  //   tx_intf_api->TX_INTF_REG_CFG_DATA_TO_ANT_write(0x100); //slv_reg10[8] -- bit 8 not used anymore. only bit0/1 are still reserved. 
  // }

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
  tx_intf_api->TX_INTF_REG_ARBITRARY_IQ_read=TX_INTF_REG_ARBITRARY_IQ_read;
  tx_intf_api->TX_INTF_REG_WIFI_TX_MODE_read=TX_INTF_REG_WIFI_TX_MODE_read;
  tx_intf_api->TX_INTF_REG_CTS_TOSELF_CONFIG_read=TX_INTF_REG_CTS_TOSELF_CONFIG_read;
  tx_intf_api->TX_INTF_REG_CSI_FUZZER_read=TX_INTF_REG_CSI_FUZZER_read;
  tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_read=TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_read;
  tx_intf_api->TX_INTF_REG_ARBITRARY_IQ_CTL_read=TX_INTF_REG_ARBITRARY_IQ_CTL_read;
  tx_intf_api->TX_INTF_REG_TX_CONFIG_read=TX_INTF_REG_TX_CONFIG_read;
  tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read=TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read;
  tx_intf_api->TX_INTF_REG_CFG_DATA_TO_ANT_read=TX_INTF_REG_CFG_DATA_TO_ANT_read;
  tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_TH_read=TX_INTF_REG_S_AXIS_FIFO_TH_read;
  tx_intf_api->TX_INTF_REG_TX_HOLD_THRESHOLD_read=TX_INTF_REG_TX_HOLD_THRESHOLD_read;
  tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_read=TX_INTF_REG_INTERRUPT_SEL_read;
  tx_intf_api->TX_INTF_REG_AMPDU_ACTION_CONFIG_read=TX_INTF_REG_AMPDU_ACTION_CONFIG_read;
  tx_intf_api->TX_INTF_REG_BB_GAIN_read=TX_INTF_REG_BB_GAIN_read;
  tx_intf_api->TX_INTF_REG_ANT_SEL_read=TX_INTF_REG_ANT_SEL_read;
  tx_intf_api->TX_INTF_REG_PHY_HDR_CONFIG_read=TX_INTF_REG_PHY_HDR_CONFIG_read;
  tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read=TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read;
  tx_intf_api->TX_INTF_REG_PKT_INFO1_read=TX_INTF_REG_PKT_INFO1_read;
  tx_intf_api->TX_INTF_REG_PKT_INFO2_read=TX_INTF_REG_PKT_INFO2_read;
  tx_intf_api->TX_INTF_REG_PKT_INFO3_read=TX_INTF_REG_PKT_INFO3_read;
  tx_intf_api->TX_INTF_REG_PKT_INFO4_read=TX_INTF_REG_PKT_INFO4_read;
  tx_intf_api->TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read=TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read;

  tx_intf_api->TX_INTF_REG_MULTI_RST_write=TX_INTF_REG_MULTI_RST_write;
  tx_intf_api->TX_INTF_REG_ARBITRARY_IQ_write=TX_INTF_REG_ARBITRARY_IQ_write;
  tx_intf_api->TX_INTF_REG_WIFI_TX_MODE_write=TX_INTF_REG_WIFI_TX_MODE_write;
  tx_intf_api->TX_INTF_REG_CTS_TOSELF_CONFIG_write=TX_INTF_REG_CTS_TOSELF_CONFIG_write;
  tx_intf_api->TX_INTF_REG_CSI_FUZZER_write=TX_INTF_REG_CSI_FUZZER_write;
  tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write=TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write;
  tx_intf_api->TX_INTF_REG_ARBITRARY_IQ_CTL_write=TX_INTF_REG_ARBITRARY_IQ_CTL_write;
  tx_intf_api->TX_INTF_REG_TX_CONFIG_write=TX_INTF_REG_TX_CONFIG_write;
  tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write=TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write;
  tx_intf_api->TX_INTF_REG_CFG_DATA_TO_ANT_write=TX_INTF_REG_CFG_DATA_TO_ANT_write;
  tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_TH_write=TX_INTF_REG_S_AXIS_FIFO_TH_write;
  tx_intf_api->TX_INTF_REG_TX_HOLD_THRESHOLD_write=TX_INTF_REG_TX_HOLD_THRESHOLD_write;
  tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write=TX_INTF_REG_INTERRUPT_SEL_write;
  tx_intf_api->TX_INTF_REG_AMPDU_ACTION_CONFIG_write=TX_INTF_REG_AMPDU_ACTION_CONFIG_write;
  tx_intf_api->TX_INTF_REG_BB_GAIN_write=TX_INTF_REG_BB_GAIN_write;
  tx_intf_api->TX_INTF_REG_ANT_SEL_write=TX_INTF_REG_ANT_SEL_write;
  tx_intf_api->TX_INTF_REG_PHY_HDR_CONFIG_write=TX_INTF_REG_PHY_HDR_CONFIG_write;
  tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_write=TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_write;
  tx_intf_api->TX_INTF_REG_PKT_INFO1_write=TX_INTF_REG_PKT_INFO1_write;
  tx_intf_api->TX_INTF_REG_PKT_INFO2_write=TX_INTF_REG_PKT_INFO2_write;
  tx_intf_api->TX_INTF_REG_PKT_INFO3_write=TX_INTF_REG_PKT_INFO3_write;
  tx_intf_api->TX_INTF_REG_PKT_INFO4_write=TX_INTF_REG_PKT_INFO4_write;

  /* Request and map I/O memory */
  io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  base_addr = devm_ioremap_resource(&pdev->dev, io);
  if (IS_ERR(base_addr))
    return PTR_ERR(base_addr);

  printk("%s dev_probe io start 0x%08x end 0x%08x name %s flags 0x%08x desc 0x%08x\n", tx_intf_compatible_str,io->start,io->end,io->name,(u32)io->flags,(u32)io->desc);
  printk("%s dev_probe base_addr 0x%p\n", tx_intf_compatible_str,(void*)base_addr);
  printk("%s dev_probe tx_intf_driver_api_inst 0x%p\n", tx_intf_compatible_str, (void*)(&tx_intf_driver_api_inst) );
  printk("%s dev_probe             tx_intf_api 0x%p\n", tx_intf_compatible_str, (void*)tx_intf_api);

  printk("%s dev_probe succeed!\n", tx_intf_compatible_str);

  //err = hw_init(TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1, 8, 8, SMALL_FPGA);
  //err = hw_init(TX_INTF_BYPASS, 8, 8, SMALL_FPGA);
  err = hw_init(TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1, 8, 8, SMALL_FPGA); // make sure dac is connected to original ad9361 dma

  return err;
}

static int dev_remove(struct platform_device *pdev)
{
  printk("\n");

  printk("%s dev_remove base_addr 0x%p\n", tx_intf_compatible_str,(void*)base_addr);
  printk("%s dev_remove tx_intf_driver_api_inst 0x%p\n", tx_intf_compatible_str, (void*)(&tx_intf_driver_api_inst) );
  printk("%s dev_remove             tx_intf_api 0x%p\n", tx_intf_compatible_str, (void*)tx_intf_api);

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
