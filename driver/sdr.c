// Author: Xianjun Jiao, Michael Mehari, Wei Liu, Jetmir Haxhibeqiri, Pablo Avila Campos
// SPDX-FileCopyrightText: 2022 UGent
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <linux/bitops.h>
#include <linux/dmapool.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io-64-nonatomic-lo-hi.h>

#include <linux/delay.h>
#include <linux/interrupt.h>

#include <linux/dmaengine.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/etherdevice.h>

#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched/task.h>
#include <linux/dma/xilinx_dma.h>
#include <linux/spi/spi.h>
#include <net/mac80211.h>

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#include <linux/gpio.h>
#include <linux/leds.h>

// #include <linux/time.h>

#define IIO_AD9361_USE_PRIVATE_H_
#include <../../drivers/iio/adc/ad9361_regs.h>
#include <../../drivers/iio/adc/ad9361.h>
#include <../../drivers/iio/adc/ad9361_private.h>
#include <../../drivers/iio/frequency/cf_axi_dds.h>

#include "../user_space/sdrctl_src/nl80211_testmode_def.h"
#include "hw_def.h"
#include "sdr.h"
#include "git_rev.h"

#ifndef RFSoC4x2
extern int ad9361_do_calib_run(struct ad9361_rf_phy *phy, u32 cal, int arg); 
extern int cf_axi_dds_datasel(struct cf_axi_dds_state *st, int channel, enum dds_data_select sel);
extern struct ad9361_rf_phy* ad9361_spi_to_phy(struct spi_device *spi);
extern int ad9361_tx_mute(struct ad9361_rf_phy *phy, u32 state);
extern int ad9361_ctrl_outs_setup(struct ad9361_rf_phy *phy, struct ctrl_outs_control *ctrl);
extern int ad9361_set_tx_atten(struct ad9361_rf_phy *phy, u32 atten_mdb, bool tx1, bool tx2, bool immed);
extern int ad9361_spi_read(struct spi_device *spi, u32 reg);
extern int ad9361_get_tx_atten(struct ad9361_rf_phy *phy, u32 tx_num);
#else
int ad9361_do_calib_run(struct ad9361_rf_phy *phy, u32 cal, int arg){return(0);}; 
int cf_axi_dds_datasel(struct cf_axi_dds_state *st, int channel, enum dds_data_select sel){return(0);}; 
struct ad9361_rf_phy* ad9361_spi_to_phy(struct spi_device *spi){return(0);}; 
int ad9361_tx_mute(struct ad9361_rf_phy *phy, u32 state){return(0);}; 
int ad9361_ctrl_outs_setup(struct ad9361_rf_phy *phy, struct ctrl_outs_control *ctrl){return(0);}; 
int ad9361_set_tx_atten(struct ad9361_rf_phy *phy, u32 atten_mdb, bool tx1, bool tx2, bool immed){return(0);}; 
int ad9361_spi_read(struct spi_device *spi, u32 reg){return(0);}; 
int ad9361_get_tx_atten(struct ad9361_rf_phy *phy, u32 tx_num){return(0);};
#endif
static struct ad9361_rf_phy ad9361_phy_fake;
static struct ad9361_rf_phy_state ad9361_phy_state_fake;

// driver API of component driver
extern struct tx_intf_driver_api *tx_intf_api;
extern struct rx_intf_driver_api *rx_intf_api;
extern struct openofdm_tx_driver_api *openofdm_tx_api;
extern struct openofdm_rx_driver_api *openofdm_rx_api;
extern struct xpu_driver_api *xpu_api;

u32 gen_mpdu_crc(u8 *data_in, u32 num_bytes);
u8 gen_mpdu_delim_crc(u16 m);
u32 reverse32(u32 d);
static int openwifi_set_antenna(struct ieee80211_hw *dev, u32 tx_ant, u32 rx_ant);
static int openwifi_get_antenna(struct ieee80211_hw *dev, u32 *tx_ant, u32 *rx_ant);
int rssi_half_db_to_rssi_dbm(int rssi_half_db, int rssi_correction);
int rssi_dbm_to_rssi_half_db(int rssi_dbm, int rssi_correction);
int rssi_correction_lookup_table(u32 freq_MHz);
void ad9361_tx_calibration(struct openwifi_priv *priv, u32 actual_tx_lo);
void openwifi_rf_rx_update_after_tuning(struct openwifi_priv *priv, u32 actual_rx_lo);
static void ad9361_rf_set_channel(struct ieee80211_hw *dev, struct ieee80211_conf *conf);
static void rfsoc_rf_set_channel(struct ieee80211_hw *dev, struct ieee80211_conf *conf);

#include "sdrctl_intf.c"
#include "sysfs_intf.c"

// bit0: aggregation enable(1)/disable(0); 
// bit1: tx offset tuning enable(0)/disable(1). NO USE ANY MORE
// bit1: short GI enable(1)/disable(0); 
static int test_mode = 0; 
// Internal indication variables after parsing test_mode
static bool AGGR_ENABLE = false;
static bool TX_OFFSET_TUNING_ENABLE = false;

static int init_tx_att = 0;

MODULE_AUTHOR("Xianjun Jiao");
MODULE_DESCRIPTION("SDR driver");
MODULE_LICENSE("GPL v2");

module_param(test_mode, int, 0);
MODULE_PARM_DESC(myint, "test_mode. bit0: aggregation enable(1)/disable(0)");

module_param(init_tx_att, int, 0);
MODULE_PARM_DESC(myint, "init_tx_att. TX attenuation in dB*1000  example: set to 3000 for 3dB attenuation");

// ---------------rfkill---------------------------------------
static bool openwifi_is_radio_enabled(struct openwifi_priv *priv)
{
  int reg;

  if (priv->tx_intf_cfg==TX_INTF_BW_20MHZ_AT_0MHZ_ANT0 || priv->tx_intf_cfg==TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0 || priv->tx_intf_cfg==TX_INTF_BW_20MHZ_AT_0MHZ_ANT_BOTH)
    reg = ad9361_get_tx_atten(priv->ad9361_phy, 1);
  else
    reg = ad9361_get_tx_atten(priv->ad9361_phy, 2);

  // if (reg == (AD9361_RADIO_ON_TX_ATT+priv->rf_reg_val[RF_TX_REG_IDX_ATT]))
  if (reg < AD9361_RADIO_OFF_TX_ATT)
    return true;// 0 off, 1 on
  return false;
}

void openwifi_rfkill_init(struct ieee80211_hw *hw)
{
  struct openwifi_priv *priv = hw->priv;

  priv->rfkill_off = openwifi_is_radio_enabled(priv);
  printk("%s openwifi_rfkill_init: wireless switch is %s\n", sdr_compatible_str, priv->rfkill_off ? "on" : "off");
  wiphy_rfkill_set_hw_state(hw->wiphy, !priv->rfkill_off);
  wiphy_rfkill_start_polling(hw->wiphy);
}

void openwifi_rfkill_poll(struct ieee80211_hw *hw)
{
  bool enabled;
  struct openwifi_priv *priv = hw->priv;

  enabled = openwifi_is_radio_enabled(priv);
  // printk("%s openwifi_rfkill_poll: wireless radio switch turned %s\n", sdr_compatible_str, enabled ? "on" : "off");
  if (unlikely(enabled != priv->rfkill_off)) {
    priv->rfkill_off = enabled;
    printk("%s openwifi_rfkill_poll: WARNING wireless radio switch turned %s\n", sdr_compatible_str, enabled ? "on" : "off");
    wiphy_rfkill_set_hw_state(hw->wiphy, !enabled);
  }
}

void openwifi_rfkill_exit(struct ieee80211_hw *hw)
{
  printk("%s openwifi_rfkill_exit\n", sdr_compatible_str);
  wiphy_rfkill_stop_polling(hw->wiphy);
}
//----------------rfkill end-----------------------------------

inline int rssi_dbm_to_rssi_half_db(int rssi_dbm, int rssi_correction)
{
  return ((rssi_correction+rssi_dbm)<<1);
}

inline u8 freq_MHz_to_band(u32 freq_MHz)
{
  //we choose 3822=(5160+2484)/2 for calibration to avoid treating 5140 as 2.4GHz
  if (freq_MHz < OPENWIFI_FREQ_MHz_TH_FOR_2_4GHZ_5GHZ) {
    return(BAND_2_4GHZ);
  } else {//use this BAND_5_8GHZ to represent all frequencies above OPENWIFI_FREQ_TH_FOR_2_4GHZ_5GHZ
    return(BAND_5_8GHZ);
  }
}

inline int rssi_correction_lookup_table(u32 freq_MHz)
{
  int rssi_correction;

  if (freq_MHz<2412) {
    rssi_correction = 153;
  } else if (freq_MHz<=2484) {
    rssi_correction = 153;
  // } else if (freq_MHz<5160) {
  } else if (freq_MHz<OPENWIFI_FREQ_MHz_TH_FOR_2_4GHZ_5GHZ) { //use middle point (5160+2484)/2 for calibration to avoid treating 5140 as 2.4GHz
    rssi_correction = 153;
  } else if (freq_MHz<=5240) {
    rssi_correction = 145;
  } else if (freq_MHz<=5320) {
    rssi_correction = 145;
  } else {
    rssi_correction = 145;
  }

  return rssi_correction;
}

inline void ad9361_tx_calibration(struct openwifi_priv *priv, u32 actual_tx_lo)
{
  // struct timespec64 tv;
  // unsigned long time_before = 0; 
  // unsigned long time_after = 0;
  u32 spi_disable; 

  priv->last_tx_quad_cal_lo = actual_tx_lo; 
  // do_gettimeofday(&tv);
  // time_before = tv.tv_usec + ((u64)1000000ull)*((u64)tv.tv_sec );
  spi_disable = xpu_api->XPU_REG_SPI_DISABLE_read(); // backup current fpga spi disable state
  xpu_api->XPU_REG_SPI_DISABLE_write(1); // disable FPGA SPI module
  ad9361_do_calib_run(priv->ad9361_phy, TX_QUAD_CAL, (int)priv->ad9361_phy->state->last_tx_quad_cal_phase); 
  xpu_api->XPU_REG_SPI_DISABLE_write(spi_disable); // restore original SPI disable state 
  // do_gettimeofday(&tv);
  // time_after = tv.tv_usec + ((u64)1000000ull)*((u64)tv.tv_sec );

  // printk("%s ad9361_tx_calibration %dMHz tx_quad_cal duration %lu us\n", sdr_compatible_str, actual_tx_lo, time_after-time_before);
  printk("%s ad9361_tx_calibration %dMHz tx_quad_cal duration unknown us\n", sdr_compatible_str, actual_tx_lo);
}

inline void openwifi_rf_rx_update_after_tuning(struct openwifi_priv *priv, u32 actual_rx_lo)
{
  int static_lbt_th, auto_lbt_th, fpga_lbt_th, receiver_rssi_dbm_th, receiver_rssi_th;

  // get rssi correction value from lookup table
  priv->rssi_correction = rssi_correction_lookup_table(actual_rx_lo);

  // set appropriate lbt threshold
  auto_lbt_th = rssi_dbm_to_rssi_half_db(-62, priv->rssi_correction); // -62dBm
  static_lbt_th = rssi_dbm_to_rssi_half_db(-(priv->drv_xpu_reg_val[DRV_XPU_REG_IDX_LBT_TH]), priv->rssi_correction);
  fpga_lbt_th = (priv->drv_xpu_reg_val[DRV_XPU_REG_IDX_LBT_TH]==0?auto_lbt_th:static_lbt_th);
  xpu_api->XPU_REG_LBT_TH_write(fpga_lbt_th);
  priv->last_auto_fpga_lbt_th = auto_lbt_th;

  // Set rssi_half_db threshold (-85dBm equivalent) to receiver. Receiver will not react to signal lower than this rssi. See test records (OPENOFDM_RX_POWER_THRES_INIT in hw_def.h)
  receiver_rssi_dbm_th = (priv->drv_rx_reg_val[DRV_RX_REG_IDX_DEMOD_TH]==0?OPENOFDM_RX_RSSI_DBM_TH_DEFAULT:(-priv->drv_rx_reg_val[DRV_RX_REG_IDX_DEMOD_TH]));
  receiver_rssi_th = rssi_dbm_to_rssi_half_db(receiver_rssi_dbm_th, priv->rssi_correction);
  openofdm_rx_api->OPENOFDM_RX_REG_POWER_THRES_write((OPENOFDM_RX_DC_RUNNING_SUM_TH_INIT<<16)|receiver_rssi_th);

  xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16)|actual_rx_lo );

  printk("%s openwifi_rf_rx_update_after_tuning %dMHz rssi_correction %d fpga_lbt_th %d(%ddBm) auto %d static %d receiver th %d(%ddBm)\n", sdr_compatible_str,
  actual_rx_lo, priv->rssi_correction, fpga_lbt_th, rssi_half_db_to_rssi_dbm(fpga_lbt_th, priv->rssi_correction), auto_lbt_th, static_lbt_th, receiver_rssi_th, receiver_rssi_dbm_th);
}

static void rfsoc_rf_set_channel(struct ieee80211_hw *dev,
          struct ieee80211_conf *conf)
{

}

static void ad9361_rf_set_channel(struct ieee80211_hw *dev,
          struct ieee80211_conf *conf)
{

  struct openwifi_priv *priv = dev->priv;
  u32 actual_rx_lo;
  u32 actual_tx_lo;
  u32 diff_tx_lo; 
  bool change_flag;

  actual_rx_lo = conf->chandef.chan->center_freq - priv->rx_freq_offset_to_lo_MHz;
  change_flag = (actual_rx_lo != priv->actual_rx_lo);

  printk("%s ad9361_rf_set_channel target %dMHz rx offset %dMHz current %dMHz change flag %d\n", sdr_compatible_str, 
  conf->chandef.chan->center_freq, priv->rx_freq_offset_to_lo_MHz, priv->actual_rx_lo, change_flag);

  // if (change_flag && priv->rf_reg_val[RF_TX_REG_IDX_FREQ_MHZ]==0 && priv->rf_reg_val[RF_RX_REG_IDX_FREQ_MHZ]==0) {
  if (change_flag) {
    actual_tx_lo = conf->chandef.chan->center_freq - priv->tx_freq_offset_to_lo_MHz;
    diff_tx_lo = priv->last_tx_quad_cal_lo > actual_tx_lo ? priv->last_tx_quad_cal_lo - actual_tx_lo : actual_tx_lo - priv->last_tx_quad_cal_lo;

    printk("%s ad9361_rf_set_channel target %dMHz tx offset %dMHz current %dMHz diff_tx_lo %dMHz\n", sdr_compatible_str, 
    conf->chandef.chan->center_freq, priv->tx_freq_offset_to_lo_MHz, priv->actual_tx_lo, diff_tx_lo);

    // -------------------Tx Lo tuning-------------------
    clk_set_rate(priv->ad9361_phy->clks[TX_RFPLL], ( ((u64)1000000ull)*((u64)actual_tx_lo) )>>1);
    priv->actual_tx_lo = actual_tx_lo;

    // -------------------Rx Lo tuning-------------------
    clk_set_rate(priv->ad9361_phy->clks[RX_RFPLL], ( ((u64)1000000ull)*((u64)actual_rx_lo) )>>1);
    priv->actual_rx_lo = actual_rx_lo;

    priv->band = freq_MHz_to_band(actual_rx_lo);
    
    // call Tx Quadrature calibration if frequency change is more than 100MHz 
    if (diff_tx_lo > 100)
      ad9361_tx_calibration(priv, actual_tx_lo);

    openwifi_rf_rx_update_after_tuning(priv, actual_rx_lo);
    printk("%s ad9361_rf_set_channel %dMHz done\n", sdr_compatible_str,conf->chandef.chan->center_freq);
  }
}

const struct openwifi_rf_ops ad9361_rf_ops = {
  .name    = "ad9361",
//  .init    = ad9361_rf_init,
//  .stop    = ad9361_rf_stop,
  .set_chan  = ad9361_rf_set_channel,
//  .calc_rssi  = ad9361_rf_calc_rssi,
};

const struct openwifi_rf_ops rfsoc4x2_rf_ops = {
  .name    = "rfsoc4x2",
//  .init    = ad9361_rf_init,
//  .stop    = ad9361_rf_stop,
  .set_chan  = rfsoc_rf_set_channel,
//  .calc_rssi  = ad9361_rf_calc_rssi,
};

u16 reverse16(u16 d) {
  union u16_byte2 tmp0, tmp1;
  tmp0.a = d;
  tmp1.c[0] = tmp0.c[1];
  tmp1.c[1] = tmp0.c[0];
  return(tmp1.a);
}

u32 reverse32(u32 d) {
  union u32_byte4 tmp0, tmp1;
  tmp0.a = d;
  tmp1.c[0] = tmp0.c[3];
  tmp1.c[1] = tmp0.c[2];
  tmp1.c[2] = tmp0.c[1];
  tmp1.c[3] = tmp0.c[0];
  return(tmp1.a);
}

static int openwifi_init_tx_ring(struct openwifi_priv *priv, int ring_idx)
{
  struct openwifi_ring *ring = &(priv->tx_ring[ring_idx]);
  int i;

  ring->stop_flag = -1;
  ring->bd_wr_idx = 0;
  ring->bd_rd_idx = 0;
  ring->bds = kmalloc(sizeof(struct openwifi_buffer_descriptor)*NUM_TX_BD,GFP_KERNEL);
  if (ring->bds==NULL) {
    printk("%s openwifi_init_tx_ring: WARNING Cannot allocate TX ring\n",sdr_compatible_str);
    return -ENOMEM;
  }

  for (i = 0; i < NUM_TX_BD; i++) {
    ring->bds[i].skb_linked=NULL; // for tx, skb is from upper layer
    //at first right after skb allocated, head, data, tail are the same.
    ring->bds[i].dma_mapping_addr = 0; // for tx, mapping is done after skb is received from upper layer in tx routine
    ring->bds[i].seq_no = 0xffff; // invalid value
    ring->bds[i].prio = 0xff; // invalid value
    ring->bds[i].len_mpdu = 0; // invalid value
  }

  return 0;
}

static void openwifi_free_tx_ring(struct openwifi_priv *priv, int ring_idx)
{
  struct openwifi_ring *ring = &(priv->tx_ring[ring_idx]);
  int i;

  ring->stop_flag = -1;
  ring->bd_wr_idx = 0;
  ring->bd_rd_idx = 0;
  for (i = 0; i < NUM_TX_BD; i++) {
    if (ring->bds[i].skb_linked == 0 && ring->bds[i].dma_mapping_addr == 0)
      continue;
    if (ring->bds[i].dma_mapping_addr != 0)
      dma_unmap_single(priv->tx_chan->device->dev, ring->bds[i].dma_mapping_addr,ring->bds[i].skb_linked->len, DMA_MEM_TO_DEV);
//    if (ring->bds[i].skb_linked!=NULL)
//      dev_kfree_skb(ring->bds[i].skb_linked); // only use dev_kfree_skb when there is exception
    if ( (ring->bds[i].dma_mapping_addr != 0 && ring->bds[i].skb_linked == 0) ||
         (ring->bds[i].dma_mapping_addr == 0 && ring->bds[i].skb_linked != 0))
      printk("%s openwifi_free_tx_ring: WARNING ring %d i %d skb_linked %p dma_mapping_addr %08x\n", sdr_compatible_str, 
      ring_idx, i, (void*)(ring->bds[i].skb_linked), (unsigned int)(ring->bds[i].dma_mapping_addr));

    ring->bds[i].skb_linked=NULL;
    ring->bds[i].dma_mapping_addr = 0;
    ring->bds[i].seq_no = 0xffff; // invalid value
    ring->bds[i].prio = 0xff; // invalid value
    ring->bds[i].len_mpdu = 0; // invalid value
  }
  if (ring->bds)
    kfree(ring->bds);
  ring->bds = NULL;
}

static int openwifi_init_rx_ring(struct openwifi_priv *priv)
{
  int i;
  u8 *pdata_tmp;

  priv->rx_cyclic_buf = dma_alloc_coherent(priv->rx_chan->device->dev,RX_BD_BUF_SIZE*NUM_RX_BD,&priv->rx_cyclic_buf_dma_mapping_addr,GFP_KERNEL);
  if (!priv->rx_cyclic_buf) {
    printk("%s openwifi_init_rx_ring: WARNING dma_alloc_coherent failed!\n", sdr_compatible_str);
    dma_free_coherent(priv->rx_chan->device->dev,RX_BD_BUF_SIZE*NUM_RX_BD,priv->rx_cyclic_buf,priv->rx_cyclic_buf_dma_mapping_addr);
    return(-1);
  }

  // Set tsft_low and tsft_high to 0. If they are not zero, it means there is a packet in the buffer by DMA
  for (i=0; i<NUM_RX_BD; i++) {
    pdata_tmp = priv->rx_cyclic_buf + i*RX_BD_BUF_SIZE; // our header insertion is at the beginning
    (*((u16*)(pdata_tmp+10))) = 0;
  }
  printk("%s openwifi_init_rx_ring: NUM_RX_BD %d RX_BD_BUF_SIZE %d pkt existing flag are cleared!\n", sdr_compatible_str,
  NUM_RX_BD, RX_BD_BUF_SIZE);

  return 0;
}

static void openwifi_free_rx_ring(struct openwifi_priv *priv)
{
  if (priv->rx_cyclic_buf)
    dma_free_coherent(priv->rx_chan->device->dev,RX_BD_BUF_SIZE*NUM_RX_BD,priv->rx_cyclic_buf,priv->rx_cyclic_buf_dma_mapping_addr);

  priv->rx_cyclic_buf_dma_mapping_addr = 0;
  priv->rx_cyclic_buf = 0;
}

static int rx_dma_setup(struct ieee80211_hw *dev){
  struct openwifi_priv *priv = dev->priv;
  struct dma_device *rx_dev = priv->rx_chan->device;

  priv->rxd = rx_dev->device_prep_dma_cyclic(priv->rx_chan,priv->rx_cyclic_buf_dma_mapping_addr,RX_BD_BUF_SIZE*NUM_RX_BD,RX_BD_BUF_SIZE,DMA_DEV_TO_MEM,DMA_CTRL_ACK|DMA_PREP_INTERRUPT);
  if (!(priv->rxd)) {
    openwifi_free_rx_ring(priv);
    printk("%s rx_dma_setup: WARNING rx_dev->device_prep_dma_cyclic %p\n", sdr_compatible_str, (void*)(priv->rxd));
    return(-1);
  }
  priv->rxd->callback = 0;
  priv->rxd->callback_param = 0;

  priv->rx_cookie = priv->rxd->tx_submit(priv->rxd);

  if (dma_submit_error(priv->rx_cookie)) {
    printk("%s rx_dma_setup: WARNING dma_submit_error(rx_cookie) %d\n", sdr_compatible_str, (u32)(priv->rx_cookie));
    return(-1);
  }

  dma_async_issue_pending(priv->rx_chan);
  return(0);
}

inline int rssi_half_db_to_rssi_dbm(int rssi_half_db, int rssi_correction) 
{
  int rssi_db, rssi_dbm;

  rssi_db = (rssi_half_db>>1);

  rssi_dbm = rssi_db - rssi_correction;
  
  rssi_dbm = (rssi_dbm < (-128)? (-128) : rssi_dbm);
  
  return rssi_dbm;
}

static irqreturn_t openwifi_rx_interrupt(int irq, void *dev_id)
{
  struct ieee80211_hw *dev = dev_id;
  struct openwifi_priv *priv = dev->priv;
  struct ieee80211_rx_status rx_status = {0};
  struct sk_buff *skb;
  struct ieee80211_hdr *hdr;
  u32 addr1_low32, addr2_low32=0, addr3_low32=0, len, rate_idx, tsft_low, tsft_high, loop_count=0;//, fc_di;
  bool ht_flag, short_gi, ht_aggr, ht_aggr_last;
  // u32 dma_driver_buf_idx_mod;
  u8 *pdata_tmp;
  u8 fcs_ok;//, target_buf_idx;//, phy_rx_sn_hw;
  s8 signal, phase_offset;
  u16 agc_status_and_pkt_exist_flag, rssi_half_db, addr1_high16, addr2_high16=0, addr3_high16=0, seq_no=0;
  bool content_ok, len_overflow, is_unicast;

#ifdef USE_NEW_RX_INTERRUPT
  int i;
  spin_lock(&priv->lock);
  for (i=0; i<NUM_RX_BD; i++) {
    pdata_tmp = priv->rx_cyclic_buf + i*RX_BD_BUF_SIZE;
    agc_status_and_pkt_exist_flag = (*((u16*)(pdata_tmp+10))); //check rx_intf_pl_to_m_axis.v. FPGA TODO: add pkt exist 1bit flag next to gpio_status_lock_by_sig_valid
    if ( agc_status_and_pkt_exist_flag==0 ) // no packet in the buffer
      continue;
#else
  static u8 target_buf_idx_old = 0;
  spin_lock(&priv->lock);
  while(1) { // loop all rx buffers that have new rx packets
    pdata_tmp = priv->rx_cyclic_buf + target_buf_idx_old*RX_BD_BUF_SIZE; // our header insertion is at the beginning
    agc_status_and_pkt_exist_flag = (*((u16*)(pdata_tmp+10)));
    if ( agc_status_and_pkt_exist_flag==0 ) // no packet in the buffer
      break;
#endif

    tsft_low =     (*((u32*)(pdata_tmp+0 )));
    tsft_high =    (*((u32*)(pdata_tmp+4 )));
    rssi_half_db = (*((u16*)(pdata_tmp+8 )));
    len =          (*((u16*)(pdata_tmp+12)));

    len_overflow = (len>(RX_BD_BUF_SIZE-16)?true:false);

    rate_idx =     (*((u16*)(pdata_tmp+14)));
    ht_flag  =     ((rate_idx&0x10)!=0);
    short_gi =     ((rate_idx&0x20)!=0);
    ht_aggr  =     (ht_flag & ((rate_idx&0x40)!=0));
    ht_aggr_last = (ht_flag & ((rate_idx&0x80)!=0));
    phase_offset = (rate_idx>>8);
    rate_idx =     (rate_idx&0x1F);

    fcs_ok = ( len_overflow?0:(*(( u8*)(pdata_tmp+16+len-1))) );

    //phy_rx_sn_hw = (fcs_ok&(NUM_RX_BD-1));
    // phy_rx_sn_hw = (fcs_ok&0x7f);//0x7f is FPGA limitation
    // dma_driver_buf_idx_mod = (state.residue&0x7f);
    fcs_ok = ((fcs_ok&0x80)!=0);

    if ( (len>=14 && (!len_overflow)) && (rate_idx>=8 && rate_idx<=23)) {
      // if ( phy_rx_sn_hw!=dma_driver_buf_idx_mod) {
      //   printk("%s openwifi_rx: WARNING sn %d next buf_idx %d!\n", sdr_compatible_str,phy_rx_sn_hw,dma_driver_buf_idx_mod);
      // }
      content_ok = true;
    } else {
      printk("%s openwifi_rx: WARNING content! len%d overflow%d rate_idx%d\n", sdr_compatible_str, 
      len, len_overflow, rate_idx);
      content_ok = false;
    }

    signal = rssi_half_db_to_rssi_dbm(rssi_half_db, priv->rssi_correction);

    hdr = (struct ieee80211_hdr *)(pdata_tmp+16);
    if (len>=20) {
      addr2_low32  = *((u32*)(hdr->addr2+2));
      addr2_high16 = *((u16*)(hdr->addr2));
    }

    addr1_low32  = *((u32*)(hdr->addr1+2));
    addr1_high16 = *((u16*)(hdr->addr1));

    if ( priv->drv_rx_reg_val[DRV_RX_REG_IDX_PRINT_CFG]&DMESG_LOG_ANY ) {
      if (len>=26) {
        addr3_low32  = *((u32*)(hdr->addr3+2));
        addr3_high16 = *((u16*)(hdr->addr3));
      }
      if (len>=28)
        seq_no = ( (hdr->seq_ctrl&IEEE80211_SCTL_SEQ)>>4 );

      is_unicast = (addr1_low32!=0xffffffff || addr1_high16!=0xffff);

      if ( (( is_unicast)&&(priv->drv_rx_reg_val[DRV_RX_REG_IDX_PRINT_CFG]&DMESG_LOG_UNICAST))   ||
           ((!is_unicast)&&(priv->drv_rx_reg_val[DRV_RX_REG_IDX_PRINT_CFG]&DMESG_LOG_BROADCAST)) ||
         ((  fcs_ok==0)&&(priv->drv_rx_reg_val[DRV_RX_REG_IDX_PRINT_CFG]&DMESG_LOG_ERROR)) )
        printk("%s openwifi_rx: %dB ht%daggr%d/%d sgi%d %dM FC%04x DI%04x ADDR%04x%08x/%04x%08x/%04x%08x SC%d fcs%d buf_idx%d %ddBm fo %d\n", sdr_compatible_str,
          len, ht_flag, ht_aggr, ht_aggr_last, short_gi, wifi_rate_table[rate_idx], hdr->frame_control, hdr->duration_id, 
          reverse16(addr1_high16), reverse32(addr1_low32), reverse16(addr2_high16), reverse32(addr2_low32), reverse16(addr3_high16), reverse32(addr3_low32), 
#ifdef USE_NEW_RX_INTERRUPT
          seq_no, fcs_ok, i, signal, phase_offset);
#else
          seq_no, fcs_ok, target_buf_idx_old, signal, phase_offset);
#endif
    }
    
    // priv->phy_rx_sn_hw_old = phy_rx_sn_hw;
    if (content_ok) {
      skb = dev_alloc_skb(len);
      if (skb) {
        skb_put_data(skb,pdata_tmp+16,len);

        rx_status.antenna = priv->runtime_rx_ant_cfg;
        // def in ieee80211_rate openwifi_rates 0~11. 0~3 11b(1M~11M), 4~11 11a/g(6M~54M)
        rx_status.rate_idx = wifi_rate_table_mapping[rate_idx];
        rx_status.signal = signal;

        rx_status.freq = dev->conf.chandef.chan->center_freq;
        // rx_status.freq = priv->actual_rx_lo;
        rx_status.band = dev->conf.chandef.chan->band;
        // rx_status.band = (rx_status.freq<2500?NL80211_BAND_2GHZ:NL80211_BAND_5GHZ);
        
        rx_status.mactime = ( ( (u64)tsft_low ) | ( ((u64)tsft_high)<<32 ) );
        rx_status.flag |= RX_FLAG_MACTIME_START;
        if (!fcs_ok)
          rx_status.flag |= RX_FLAG_FAILED_FCS_CRC;
        if (rate_idx <= 15)
          rx_status.encoding = RX_ENC_LEGACY;
        else
          rx_status.encoding = RX_ENC_HT;
        rx_status.bw = RATE_INFO_BW_20;
        if (short_gi)
          rx_status.enc_flags |= RX_ENC_FLAG_SHORT_GI;
        if(ht_aggr)
        {
          rx_status.ampdu_reference = priv->ampdu_reference;
          rx_status.flag |= RX_FLAG_AMPDU_DETAILS | RX_FLAG_AMPDU_LAST_KNOWN;
          if (ht_aggr_last)
            rx_status.flag |= RX_FLAG_AMPDU_IS_LAST;
        }

        memcpy(IEEE80211_SKB_RXCB(skb), &rx_status, sizeof(rx_status)); // put rx_status into skb->cb, from now on skb->cb is not dma_dsts any more.
        ieee80211_rx_irqsafe(dev, skb); // call mac80211 function

        // printk("%s openwifi_rx: addr1_low32 %08x self addr %08x\n", sdr_compatible_str, addr1_low32, ( *( (u32*)(priv->mac_addr+2) ) ));
        if (addr1_low32 == ( *( (u32*)(priv->mac_addr+2) ) ) && priv->stat.stat_enable) {
          agc_status_and_pkt_exist_flag = (agc_status_and_pkt_exist_flag&0x7f);
          if (len>=20) {// rx stat
            if (addr2_low32 == priv->stat.rx_target_sender_mac_addr || priv->stat.rx_target_sender_mac_addr==0) {
              if ( ieee80211_is_data(hdr->frame_control) ) {
                priv->stat.rx_data_pkt_mcs_realtime = rate_idx;
                priv->stat.rx_data_pkt_num_total++;
                if (!fcs_ok) {
                  priv->stat.rx_data_pkt_num_fail++;
                  priv->stat.rx_data_pkt_fail_mcs_realtime = rate_idx;
                  priv->stat.rx_data_fail_agc_gain_value_realtime = agc_status_and_pkt_exist_flag;
                } else {
                  priv->stat.rx_data_ok_agc_gain_value_realtime = agc_status_and_pkt_exist_flag;
                }
              } else if ( ieee80211_is_mgmt(hdr->frame_control) ) {
                priv->stat.rx_mgmt_pkt_mcs_realtime = rate_idx;
                priv->stat.rx_mgmt_pkt_num_total++;
                if (!fcs_ok) {
                  priv->stat.rx_mgmt_pkt_num_fail++;
                  priv->stat.rx_mgmt_pkt_fail_mcs_realtime = rate_idx;
                  priv->stat.rx_mgmt_fail_agc_gain_value_realtime = agc_status_and_pkt_exist_flag;
                } else {
                  priv->stat.rx_mgmt_ok_agc_gain_value_realtime = agc_status_and_pkt_exist_flag;
                }
              }
            }
          } else if ( ieee80211_is_ack(hdr->frame_control) ) {
            priv->stat.rx_ack_pkt_mcs_realtime = rate_idx;
            priv->stat.rx_ack_pkt_num_total++;
            if (!fcs_ok) {
              priv->stat.rx_ack_pkt_num_fail++;
            } else {
              priv->stat.rx_ack_ok_agc_gain_value_realtime = agc_status_and_pkt_exist_flag;
            }
          }
        }
      } else
        printk("%s openwifi_rx: WARNING dev_alloc_skb failed!\n", sdr_compatible_str);

      if(ht_aggr_last)
        priv->ampdu_reference++;
    }
    (*((u16*)(pdata_tmp+10))) = 0; // clear the field (set by rx_intf_pl_to_m_axis.v) to indicate the packet has been processed
    loop_count++;
#ifndef USE_NEW_RX_INTERRUPT
    target_buf_idx_old=((target_buf_idx_old+1)&(NUM_RX_BD-1)); 
#endif
  }

  if ( loop_count!=1 && (priv->drv_rx_reg_val[DRV_RX_REG_IDX_PRINT_CFG]&DMESG_LOG_ERROR) )
    printk("%s openwifi_rx: WARNING loop_count %d\n", sdr_compatible_str,loop_count);
  
// openwifi_rx_out:
  spin_unlock(&priv->lock);
  return IRQ_HANDLED;
}

static irqreturn_t openwifi_tx_interrupt(int irq, void *dev_id)
{
  struct ieee80211_hw *dev = dev_id;
  struct openwifi_priv *priv = dev->priv;
  struct openwifi_ring *ring, *drv_ring_tmp;
  struct sk_buff *skb;
  struct ieee80211_tx_info *info;
  struct ieee80211_hdr *hdr;
  u32 reg_val1, hw_queue_len, reg_val2, dma_fifo_no_room_flag, num_slot_random, cw, loop_count=0, addr1_low32, mcs_for_sysfs;
  u16 seq_no, pkt_cnt, blk_ack_ssn, start_idx;
  u8 nof_retx=-1, last_bd_rd_idx, i, prio, queue_idx, nof_retx_stat;
  u64 blk_ack_bitmap;
  // u16 prio_rd_idx_store[64]={0};
  bool tx_fail=false, fpga_queue_has_room=false;
  bool use_ht_aggr, pkt_need_ack, use_ht_rate, prio_wake_up_flag = false;

  spin_lock(&priv->lock);

  while(1) { // loop all packets that have been sent by FPGA
    reg_val1 = tx_intf_api->TX_INTF_REG_PKT_INFO1_read();
        reg_val2 = tx_intf_api->TX_INTF_REG_PKT_INFO2_read();
    blk_ack_bitmap = (tx_intf_api->TX_INTF_REG_PKT_INFO3_read() | ((u64)tx_intf_api->TX_INTF_REG_PKT_INFO4_read())<<32);

    if (reg_val1!=0xFFFFFFFF) {
      nof_retx = (reg_val1&0xF);
      last_bd_rd_idx = ((reg_val1>>5)&(NUM_TX_BD-1));
      prio = ((reg_val1>>17)&0x3);
      num_slot_random = ((reg_val1>>19)&0x1FF);
      //num_slot_random = ((0xFF80000 &reg_val1)>>(2+5+NUM_BIT_MAX_PHY_TX_SN+NUM_BIT_MAX_NUM_HW_QUEUE));
      cw = ((reg_val1>>28)&0xF);
      //cw = ((0xF0000000 & reg_val1) >> 28);
      if(cw > 10) {
        cw = 10 ;
        num_slot_random += 512 ; 
      }
      pkt_cnt = (reg_val2&0x3F);
      blk_ack_ssn = ((reg_val2>>6)&0xFFF);

      queue_idx = ((reg_val1>>15)&(MAX_NUM_HW_QUEUE-1));
      dma_fifo_no_room_flag = tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read();
      hw_queue_len = tx_intf_api->TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read();
      // check which linux prio is stopped by this queue (queue_idx)
      for (i=0; i<MAX_NUM_SW_QUEUE; i++) {
        drv_ring_tmp = &(priv->tx_ring[i]);
        if ( drv_ring_tmp->stop_flag == prio ) {

          if ( ((dma_fifo_no_room_flag>>i)&1)==0 && (NUM_TX_BD-((hw_queue_len>>(i*8))&0xFF))>=RING_ROOM_THRESHOLD )
            fpga_queue_has_room=true;
          else
            fpga_queue_has_room=false;

          // Wake up Linux queue due to the current fpga queue releases some room
          if( priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG]&DMESG_LOG_NORMAL_QUEUE_STOP )
            printk("%s openwifi_tx_interrupt: WARNING ieee80211_wake_queue prio%d i%d queue%d no room flag%x hwq len%08x wr%d rd%d\n", sdr_compatible_str,
                    prio, i, queue_idx, dma_fifo_no_room_flag, hw_queue_len, drv_ring_tmp->bd_wr_idx, last_bd_rd_idx);
            
          if (fpga_queue_has_room) {
            prio_wake_up_flag = true;
            drv_ring_tmp->stop_flag = -1;

            if (priv->stat.stat_enable) {
              priv->stat.tx_prio_wakeup_num[prio]++;
              priv->stat.tx_queue_wakeup_num[i]++;
            }
          } else {
            if( priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG]&DMESG_LOG_NORMAL_QUEUE_STOP )
              printk("%s openwifi_tx_interrupt: WARNING no room! prio_wake_up_flag%d\n", sdr_compatible_str, prio_wake_up_flag);
          }
        }
      }
      if (prio_wake_up_flag)
        ieee80211_wake_queue(dev, prio);

      if (priv->stat.stat_enable) {
        priv->stat.tx_prio_interrupt_num[prio] = priv->stat.tx_prio_interrupt_num[prio] + pkt_cnt;
        priv->stat.tx_queue_interrupt_num[queue_idx] = priv->stat.tx_queue_interrupt_num[queue_idx] + pkt_cnt;
      }

      ring = &(priv->tx_ring[queue_idx]);
      for(i = 1; i <= pkt_cnt; i++)
      {
        ring->bd_rd_idx = (last_bd_rd_idx + i - pkt_cnt + 64)%64;
        seq_no = ring->bds[ring->bd_rd_idx].seq_no;

        if (seq_no == 0xffff) {// it has been forced cleared by the openwifi_tx (due to out-of-order Tx of different queues to the air?)
          printk("%s openwifi_tx_interrupt: WARNING wr%d rd%d last_bd_rd_idx%d i%d pkt_cnt%d prio%d fpga q%d hwq len%d bd prio%d len_mpdu%d seq_no%d skb_linked%p dma_mapping_addr%u\n", sdr_compatible_str,
          ring->bd_wr_idx, ring->bd_rd_idx, last_bd_rd_idx, i, pkt_cnt, prio, queue_idx, hw_queue_len, ring->bds[ring->bd_rd_idx].prio, ring->bds[ring->bd_rd_idx].len_mpdu, seq_no, ring->bds[ring->bd_rd_idx].skb_linked, (long long int)(ring->bds[ring->bd_rd_idx].dma_mapping_addr));
          continue;
        }

        skb = ring->bds[ring->bd_rd_idx].skb_linked;

        dma_unmap_single(priv->tx_chan->device->dev,ring->bds[ring->bd_rd_idx].dma_mapping_addr,
            skb->len, DMA_MEM_TO_DEV);

        info = IEEE80211_SKB_CB(skb);
        use_ht_aggr = ((info->flags&IEEE80211_TX_CTL_AMPDU)!=0);
        ieee80211_tx_info_clear_status(info);

        // Aggregation packet
        if (use_ht_aggr)
        {
          start_idx = (seq_no>=blk_ack_ssn) ? (seq_no-blk_ack_ssn) : (seq_no+((~blk_ack_ssn+1)&0x0FFF));
          tx_fail = (((blk_ack_bitmap>>start_idx)&0x1)==0);
          info->flags |= IEEE80211_TX_STAT_AMPDU;
          info->status.ampdu_len = 1;
          info->status.ampdu_ack_len = (tx_fail == true) ? 0 : 1;

          skb_pull(skb, LEN_MPDU_DELIM);
          //skb_trim(skb, num_byte_pad_skb);
        }
        // Normal packet
        else
        {
          tx_fail = ((blk_ack_bitmap&0x1)==0);
          info->flags &= (~IEEE80211_TX_CTL_AMPDU);
        }

        pkt_need_ack = (!(info->flags & IEEE80211_TX_CTL_NO_ACK));
        // do statistics for data packet that needs ack
        hdr = (struct ieee80211_hdr *)skb->data;
        addr1_low32  = *((u32*)(hdr->addr1+2));
        if ( priv->stat.stat_enable && pkt_need_ack && (addr1_low32 == priv->stat.rx_target_sender_mac_addr || priv->stat.rx_target_sender_mac_addr==0) ) {
          use_ht_rate = (((info->control.rates[0].flags)&IEEE80211_TX_RC_MCS)!=0);
          mcs_for_sysfs = ieee80211_get_tx_rate(dev, info)->hw_value;
          if (use_ht_rate)
            mcs_for_sysfs = (mcs_for_sysfs | 0x80000000);
          
          if ( ieee80211_is_data(hdr->frame_control) ) {
            nof_retx_stat = (nof_retx<=5?nof_retx:5);

            priv->stat.tx_data_pkt_need_ack_num_total++;
            priv->stat.tx_data_pkt_mcs_realtime = mcs_for_sysfs;
            priv->stat.tx_data_pkt_need_ack_num_retx[nof_retx_stat]++;
            if (tx_fail) {
              priv->stat.tx_data_pkt_need_ack_num_total_fail++;
              priv->stat.tx_data_pkt_fail_mcs_realtime = mcs_for_sysfs;
              priv->stat.tx_data_pkt_need_ack_num_retx_fail[nof_retx_stat]++;
            }
          } else if ( ieee80211_is_mgmt(hdr->frame_control) ) {
            nof_retx_stat = (nof_retx<=2?nof_retx:2);

            priv->stat.tx_mgmt_pkt_need_ack_num_total++;
            priv->stat.tx_mgmt_pkt_mcs_realtime = mcs_for_sysfs;
            priv->stat.tx_mgmt_pkt_need_ack_num_retx[nof_retx_stat]++;
            if (tx_fail) {
              priv->stat.tx_mgmt_pkt_need_ack_num_total_fail++;
              priv->stat.tx_mgmt_pkt_fail_mcs_realtime = mcs_for_sysfs;
              priv->stat.tx_mgmt_pkt_need_ack_num_retx_fail[nof_retx_stat]++;
            }
          }
        }

        if ( tx_fail == false )
          info->flags |= IEEE80211_TX_STAT_ACK;

        info->status.rates[0].count = nof_retx + 1; //according to our test, the 1st rate is the most important. we only do retry on the 1st rate
        info->status.rates[1].idx = -1;
        // info->status.rates[2].idx = -1;
        // info->status.rates[3].idx = -1;//in mac80211.h: #define IEEE80211_TX_MAX_RATES  4
        info->status.antenna = priv->runtime_tx_ant_cfg;

        if ( ( (!pkt_need_ack)&&(priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG]&DMESG_LOG_BROADCAST) ) || ( (pkt_need_ack)&&(priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG]&DMESG_LOG_UNICAST) ) ){
          printk("%s openwifi_tx_interrupt: tx_result [nof_retx %d pass %d] SC%d prio%d q%d wr%d rd%d num_slot%d cw%d hwq len%08x no_room_flag%x\n", sdr_compatible_str,
          nof_retx+1, !tx_fail, seq_no, prio, queue_idx, ring->bd_wr_idx, ring->bd_rd_idx, num_slot_random, cw, hw_queue_len, dma_fifo_no_room_flag);
        }

        ieee80211_tx_status_irqsafe(dev, skb);

        ring->bds[ring->bd_rd_idx].prio = 0xff; // invalid value
        ring->bds[ring->bd_rd_idx].len_mpdu = 0; // invalid value
        ring->bds[ring->bd_rd_idx].seq_no = 0xffff;
        ring->bds[ring->bd_rd_idx].skb_linked = NULL;
        ring->bds[ring->bd_rd_idx].dma_mapping_addr = 0;
      }
      
      loop_count++;
      
      // printk("%s openwifi_tx_interrupt: loop %d prio %d rd %d\n", sdr_compatible_str, loop_count, prio, ring->bd_rd_idx);

    } else
      break;
  }
  if ( loop_count!=1 && ((priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG])&DMESG_LOG_ERROR) )
    printk("%s openwifi_tx_interrupt: WARNING loop_count %d\n", sdr_compatible_str, loop_count);

  spin_unlock(&priv->lock);
  return IRQ_HANDLED;
}

u32 crc_table[16] = {0x4DBDF21C, 0x500AE278, 0x76D3D2D4, 0x6B64C2B0, 0x3B61B38C, 0x26D6A3E8, 0x000F9344, 0x1DB88320, 0xA005713C, 0xBDB26158, 0x9B6B51F4, 0x86DC4190, 0xD6D930AC, 0xCB6E20C8, 0xEDB71064, 0xF0000000};
u32 gen_mpdu_crc(u8 *data_in, u32 num_bytes)
{
  u32 i, crc = 0;
  u8 idx;
  for( i = 0; i < num_bytes; i++)
  {
    idx = (crc & 0x0F) ^ (data_in[i] & 0x0F);
    crc = (crc >> 4) ^ crc_table[idx];

    idx = (crc & 0x0F) ^ ((data_in[i] >> 4) & 0x0F);
    crc = (crc >> 4) ^ crc_table[idx];
  }

  return crc;
}

u8 gen_mpdu_delim_crc(u16 m)
{
  u8 i, temp, c[8] = {1, 1, 1, 1, 1, 1, 1, 1}, mpdu_delim_crc;

  for (i = 0; i < 16; i++)
  {
    temp = c[7] ^ ((m >> i) & 0x01);

    c[7] = c[6];
    c[6] = c[5];
    c[5] = c[4];
    c[4] = c[3];
    c[3] = c[2];
    c[2] = c[1] ^ temp;
    c[1] = c[0] ^ temp;
    c[0] = temp;
  }
  mpdu_delim_crc = ((~c[7] & 0x01) << 0) | ((~c[6] & 0x01) << 1) | ((~c[5] & 0x01) << 2) | ((~c[4] & 0x01) << 3) | ((~c[3] & 0x01) << 4) | ((~c[2] & 0x01) << 5) | ((~c[1] & 0x01) << 6) | ((~c[0] & 0x01) << 7);

  return mpdu_delim_crc;
}

static inline struct gpio_led_data * //please align with the implementation in leds-gpio.c
      cdev_to_gpio_led_data(struct led_classdev *led_cdev)
{
  return container_of(led_cdev, struct gpio_led_data, cdev);
}

inline int calc_n_ofdm(int num_octet, int n_dbps)
{
  int num_bit, num_ofdm_sym;
  
  num_bit      = 22+num_octet*8;
  num_ofdm_sym = (num_bit/n_dbps) + ((num_bit%n_dbps)!=0);

  return (num_ofdm_sym);
}

inline __le16 gen_ht_duration_id(__le16 frame_control, __le16 aid, u8 qos_hdr, bool use_ht_aggr, u8 rate_hw_value, u16 sifs) 
{
// COTS wifi ht QoS data duration field analysis (lots of capture):

// ht non-aggr QoS data: 44, type 2 (data frame) sub-type 8 (1000) 21.7/52/57.8/58.5/65Mbps
// ack     ht 36 + 4*[(22+14*8)/78] = 36 + 4*2 = 44
// ack legacy 20 + 4*[(22+14*8)/72] = 20 + 4*2 = 28

// ht non-aggr QoS data: 60, type 2 (data frame) sub-type 8 (1000) 6.5Mbps
// ack     ht 36 + 4*[(22+14*8)/26] = 36 + 4*6 = 60
// ack legacy 20 + 4*[(22+14*8)/24] = 20 + 4*6 = 44

// ht     aggr QoS data: 52, type 2 (data frame) sub-type 8 (1000) 19.5/28.9/39/57.8/65/72.2Mbps
// ack     ht 36 + 4*[(22+32*8)/78] = 36 + 4*4 = 52
// ack legacy 20 + 4*[(22+32*8)/72] = 20 + 4*4 = 36

// ht     aggr QoS data: 60, type 2 (data frame) sub-type 8 (1000) 13/14.4Mbps
// ack     ht 36 + 4*[(22+32*8)/52] = 36 + 4*6 = 60
// ack legacy 20 + 4*[(22+32*8)/48] = 20 + 4*6 = 44

// ht and legacy rate mapping is ont one on one, instead it is modulation combined with coding rate
// modulate  coding  ht-mcs ht-n_dbps legacy-mcs legacy-n_dbps
// BPSK      1/2     0      26        4          24
// QPSK      1/2     1      52        6          48
// QPSK      3/4     2      78        7          72
// 16QAM     1/2     3      104       8          96
// 16QAM     3/4     4      156       9          144
// 64QAM     2/3     5      208       10         192
// 64QAM     3/4     6      234       11         216

// conclusion: duration is: assume ack/blk-ack uses legacy, plus SIFS

// other observation: ht always use QoS data, not data (sub-type)
// other observation: management/control frame always in non-ht

  __le16 dur = 0;
  u16 n_dbps;
  int num_octet, num_ofdm_sym;

  if (ieee80211_is_pspoll(frame_control)) {
    dur = (aid|0xc000);
  } else if (ieee80211_is_data_qos(frame_control) && (~(qos_hdr&IEEE80211_QOS_CTL_ACK_POLICY_NOACK))) {
    rate_hw_value = (rate_hw_value>6?6:rate_hw_value);
    n_dbps = (rate_hw_value==0?wifi_n_dbps_table[4]:wifi_n_dbps_table[rate_hw_value+5]);
    num_octet = (use_ht_aggr?32:14); //32 bytes for compressed block ack; 14 bytes for normal ack
    num_ofdm_sym = calc_n_ofdm(num_octet, n_dbps);
    dur = sifs + 20 + 4*num_ofdm_sym; // 20us legacy preamble
    // printk("%s gen_ht_duration_id: num_octet %d n_dbps %d num_ofdm_sym %d dur %d\n", sdr_compatible_str, 
    // num_octet, n_dbps, num_ofdm_sym, dur);
  } else {
    printk("%s openwifi_tx: WARNING gen_ht_duration_id wrong pkt type!\n", sdr_compatible_str);
  }

  return dur;
}

inline void report_pkt_loss_due_to_driver_drop(struct ieee80211_hw *dev, struct sk_buff *skb)
{
  struct openwifi_priv *priv = dev->priv;
  struct ieee80211_tx_info *info;

  info = IEEE80211_SKB_CB(skb);
  ieee80211_tx_info_clear_status(info);
  info->status.rates[0].count = 1;
  info->status.rates[1].idx = -1;
  info->status.antenna = priv->runtime_tx_ant_cfg;
  ieee80211_tx_status_irqsafe(dev, skb);
}

static void openwifi_tx(struct ieee80211_hw *dev,
           struct ieee80211_tx_control *control,
           struct sk_buff *skb)
{
  struct openwifi_priv *priv = dev->priv;
  unsigned long flags;
  struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
  struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
  struct openwifi_ring *ring = NULL;
  struct sk_buff *skb_new; // temp skb for internal use
  struct ieee80211_tx_info *info_skipped;
  dma_addr_t dma_mapping_addr;
  unsigned int i, j, empty_bd_idx = 0;
  u16 rate_signal_value, len_mpdu, len_psdu, num_dma_symbol, len_mpdu_delim_pad=0, num_byte_pad;
  u32 num_dma_byte, addr1_low32, addr2_low32=0, addr3_low32=0, tx_config, cts_reg=0, phy_hdr_config;//, openofdm_state_history;
  u16 addr1_high16, addr2_high16=0, addr3_high16=0, sc, seq_no=0, cts_duration=0, sifs, ack_duration=0, traffic_pkt_duration, n_dbps;
  u8 cts_rate_hw_value=0, cts_rate_signal_value=0, rate_hw_value, pkt_need_ack, retry_limit_raw,use_short_gi,*dma_buf,retry_limit_hw_value,rc_flags,qos_hdr,prio,queue_idx,drv_ring_idx;
  bool drv_seqno=false, use_rts_cts, use_cts_protect, ht_aggr_start=false, use_ht_rate, use_ht_aggr, cts_use_traffic_rate=false, force_use_cts_protect=false;
  __le16 frame_control,duration_id;
  u32 dma_fifo_no_room_flag, hw_queue_len, delay_count=0;
  u16 aid = 0;
  enum dma_status status;

  static u32 addr1_low32_prev = -1;
  static u8 rate_hw_value_prev = -1;
  static u8 pkt_need_ack_prev = -1;
  static u16 addr1_high16_prev = -1;
  static __le16 duration_id_prev = -1;
  static u8 prio_prev = -1;
  static u8 retry_limit_raw_prev = -1;
  static u8 use_short_gi_prev = -1;

  // static bool led_status=0;
  // struct gpio_led_data *led_dat = cdev_to_gpio_led_data(priv->led[3]);

  // if ( (priv->phy_tx_sn&7) ==0 ) {
  //   openofdm_state_history = openofdm_rx_api->OPENOFDM_RX_REG_STATE_HISTORY_read();
  //   if (openofdm_state_history!=openofdm_state_history_old){
  //     led_status = (~led_status);
  //     openofdm_state_history_old = openofdm_state_history;
  //     gpiod_set_value(led_dat->gpiod, led_status);
  //   }
  // }

  if (skb->data_len>0) {// more data are not in linear data area skb->data
    printk("%s openwifi_tx: WARNING skb->data_len>0\n", sdr_compatible_str);
    goto openwifi_tx_early_out;
  }

  len_mpdu = skb->len;

  // get Linux priority/queue setting info and target mac address
  prio = skb_get_queue_mapping(skb);
  if (prio >= MAX_NUM_HW_QUEUE) {
    printk("%s openwifi_tx: WARNING prio%d\n", sdr_compatible_str, prio);
    goto openwifi_tx_early_out;
  }

  addr1_low32  = *((u32*)(hdr->addr1+2));

  // ---- DO your idea here! Map Linux/SW "prio" to driver "drv_ring_idx" (then 1on1 to FPGA queue_idx) ---
  if (priv->slice_idx == 0xFFFFFFFF) {// use Linux default prio setting, if there isn't any slice config
    drv_ring_idx = prio;
  } else {// customized prio to drv_ring_idx mapping
    // check current packet belonging to which slice/hw-queue
    for (i=0; i<MAX_NUM_HW_QUEUE; i++) {
      if ( priv->dest_mac_addr_queue_map[i] == addr1_low32 ) {
        break;
      }
    }
    drv_ring_idx = (i>=MAX_NUM_HW_QUEUE?prio:i); // if no address is hit
  }

  ring = &(priv->tx_ring[drv_ring_idx]);

  spin_lock_irqsave(&priv->lock, flags);
  if (ring->bds[ring->bd_wr_idx].seq_no != 0xffff) { // not cleared yet by interrupt
    for (i=1; i<NUM_TX_BD; i++) {
      if (ring->bds[(ring->bd_wr_idx+i)&(NUM_TX_BD-1)].seq_no == 0xffff) {
        empty_bd_idx = i;
        break;
      }
    }
    hw_queue_len = tx_intf_api->TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read();
    if (empty_bd_idx) { // clear all bds before the empty bd and report failure to Linux
      if (priv->stat.stat_enable) {
        priv->stat.tx_prio_stop0_fake_num[prio]++;
        priv->stat.tx_queue_stop0_fake_num[drv_ring_idx]++;
      }
      for (i=0; i<empty_bd_idx; i++) {
        j = ( (ring->bd_wr_idx+i)&(NUM_TX_BD-1) );
        printk("%s openwifi_tx: WARNING fake stop queue empty_bd_idx%d i%d lnx prio%d map to q%d stop%d hwq len%d wr%d rd%d bd prio%d len_mpdu%d seq_no%d skb_linked%p dma_mapping_addr%u\n", sdr_compatible_str,
        empty_bd_idx, i, prio, drv_ring_idx, ring->stop_flag, hw_queue_len, ring->bd_wr_idx, ring->bd_rd_idx, ring->bds[j].prio, ring->bds[j].len_mpdu, ring->bds[j].seq_no, ring->bds[j].skb_linked,(long long int)(ring->bds[j].dma_mapping_addr));
        // tell Linux this skb failed
        skb_new = ring->bds[j].skb_linked;
        dma_unmap_single(priv->tx_chan->device->dev,ring->bds[j].dma_mapping_addr,
              skb_new->len, DMA_MEM_TO_DEV);
        info_skipped = IEEE80211_SKB_CB(skb_new);
        ieee80211_tx_info_clear_status(info_skipped);
        info_skipped->status.rates[0].count = 1;
        info_skipped->status.rates[1].idx = -1;
        info_skipped->status.antenna = priv->runtime_tx_ant_cfg;
        ieee80211_tx_status_irqsafe(dev, skb_new);

        ring->bds[j].prio = 0xff; // invalid value
        ring->bds[j].len_mpdu = 0; // invalid value
        ring->bds[j].seq_no = 0xffff;
        ring->bds[j].skb_linked = NULL;
        ring->bds[j].dma_mapping_addr = 0;

      }
      if (ring->stop_flag != -1) { //the interrupt seems will never come, we need to wake up the queue in case the interrupt will never wake it up
        ieee80211_wake_queue(dev, ring->stop_flag);
        ring->stop_flag = -1;
      }
    } else {
      j = ring->bd_wr_idx;
      printk("%s openwifi_tx: WARNING real stop queue lnx prio%d map to q%d stop%d hwq len%d wr%d rd%d bd prio%d len_mpdu%d seq_no%d skb_linked%p dma_mapping_addr%u\n", sdr_compatible_str,
      prio, drv_ring_idx, ring->stop_flag, hw_queue_len, ring->bd_wr_idx, ring->bd_rd_idx, ring->bds[j].prio, ring->bds[j].len_mpdu, ring->bds[j].seq_no, ring->bds[j].skb_linked, (long long int)(ring->bds[j].dma_mapping_addr));

      ieee80211_stop_queue(dev, prio); // here we should stop those prio related to the queue idx flag set in TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read
      ring->stop_flag = prio;
      if (priv->stat.stat_enable) {
        priv->stat.tx_prio_stop0_real_num[prio]++;
        priv->stat.tx_queue_stop0_real_num[drv_ring_idx]++;
      }

      spin_unlock_irqrestore(&priv->lock, flags);
      goto openwifi_tx_early_out;
    }
  }
  spin_unlock_irqrestore(&priv->lock, flags);
  // -------------------- end of Map Linux/SW "prio" to driver "drv_ring_idx" ------------------

  // get other info from packet header
  addr1_high16 = *((u16*)(hdr->addr1));
  if (len_mpdu>=20) {
    addr2_low32  = *((u32*)(hdr->addr2+2));
    addr2_high16 = *((u16*)(hdr->addr2));
  }
  if (len_mpdu>=26) {
    addr3_low32  = *((u32*)(hdr->addr3+2));
    addr3_high16 = *((u16*)(hdr->addr3));
  }

  frame_control=hdr->frame_control;
  pkt_need_ack = (!(info->flags&IEEE80211_TX_CTL_NO_ACK));

  retry_limit_raw = info->control.rates[0].count;

  rc_flags = info->control.rates[0].flags;
  use_rts_cts = ((rc_flags&IEEE80211_TX_RC_USE_RTS_CTS)!=0);
  use_cts_protect = ((rc_flags&IEEE80211_TX_RC_USE_CTS_PROTECT)!=0);
  use_ht_rate = ((rc_flags&IEEE80211_TX_RC_MCS)!=0);
  use_short_gi = ((rc_flags&IEEE80211_TX_RC_SHORT_GI)!=0);
  use_ht_aggr = ((info->flags&IEEE80211_TX_CTL_AMPDU)!=0);
  qos_hdr = (*(ieee80211_get_qos_ctl(hdr)));

  // get Linux rate (MCS) setting
  rate_hw_value = ieee80211_get_tx_rate(dev, info)->hw_value;
  // drv_tx_reg_val[DRV_TX_REG_IDX_RATE]
  // override rate legacy: 4:6M,   5:9M,  6:12M,  7:18M, 8:24M, 9:36M, 10:48M,   11:54M
  // drv_tx_reg_val[DRV_TX_REG_IDX_RATE_HT] 
  // override rate     ht: 4:6.5M, 5:13M, 6:19.5M,7:26M, 8:39M, 9:52M, 10:58.5M, 11:65M
  if ( ieee80211_is_data(hdr->frame_control) ) {//rate override command
    if (use_ht_rate && priv->drv_tx_reg_val[DRV_TX_REG_IDX_RATE_HT]>0) {
      rate_hw_value = (priv->drv_tx_reg_val[DRV_TX_REG_IDX_RATE_HT]&0xF)-4;
      use_short_gi  = ((priv->drv_tx_reg_val[DRV_TX_REG_IDX_RATE_HT]&0x10)==0x10);
    } else if ((!use_ht_rate) && priv->drv_tx_reg_val[DRV_TX_REG_IDX_RATE]>0)
      rate_hw_value = (priv->drv_tx_reg_val[DRV_TX_REG_IDX_RATE]&0xF);
    // TODO: need to map rate_hw_value back to info->control.rates[0].idx!!!
  }

  // Workaround for a FPGA bug: if aggr happens on ht mcs 0, the tx core will never end, running eneless and stuck the low MAC!
  if (use_ht_aggr && rate_hw_value==0)
    rate_hw_value = 1;

  // sifs = (priv->actual_rx_lo<2500?10:16);
  sifs = 16; // for ofdm, sifs is always 16

  if (control != NULL) { // get aid for gen_ht_duration_id only when control->sta is not NULL
    if (control->sta != NULL) {
      aid = control->sta->aid;
    }
  }

  if (use_ht_rate) {
    // printk("%s openwifi_tx: rate_hw_value %d aggr %d sifs %d\n", sdr_compatible_str, rate_hw_value, use_ht_aggr, sifs);
    hdr->duration_id = gen_ht_duration_id(frame_control, aid, qos_hdr, use_ht_aggr, rate_hw_value, sifs); //linux only do it for 11a/g, not for 11n and later
  }
  duration_id = hdr->duration_id;

  if (use_rts_cts)
    printk("%s openwifi_tx: WARNING sn %d use_rts_cts is not supported!\n", sdr_compatible_str, ring->bd_wr_idx);

  if (use_cts_protect) {
    cts_rate_hw_value = ieee80211_get_rts_cts_rate(dev, info)->hw_value;
    cts_duration = le16_to_cpu(ieee80211_ctstoself_duration(dev,info->control.vif,len_mpdu,info));
  } else if (force_use_cts_protect) { // could override mac80211 setting here.
    cts_rate_hw_value = 4; //wifi_mcs_table_11b_force_up[] translate it to 1011(6M)
    if (pkt_need_ack)
      ack_duration = 44;//assume the ack we wait use 6Mbps: 4*ceil((22+14*8)/24) + 20(preamble+SIGNAL)
    
    n_dbps = (use_ht_rate?wifi_n_dbps_ht_table[rate_hw_value+4]:wifi_n_dbps_table[rate_hw_value]);
    traffic_pkt_duration = (use_ht_rate?36:20) + 4*calc_n_ofdm(len_mpdu, n_dbps);
    cts_duration = traffic_pkt_duration + sifs + pkt_need_ack*(sifs+ack_duration);
  }

// this is 11b stuff
//  if (info->flags&IEEE80211_TX_RC_USE_SHORT_PREAMBLE)
//    printk("%s openwifi_tx: WARNING IEEE80211_TX_RC_USE_SHORT_PREAMBLE\n", sdr_compatible_str);

  if (len_mpdu>=28) {
    if (info->flags & IEEE80211_TX_CTL_ASSIGN_SEQ) {
      if (info->flags & IEEE80211_TX_CTL_FIRST_FRAGMENT) {
        priv->seqno += 0x10;
        drv_seqno = true;
      }
      hdr->seq_ctrl &= cpu_to_le16(IEEE80211_SCTL_FRAG);
      hdr->seq_ctrl |= cpu_to_le16(priv->seqno);
    }
    sc = hdr->seq_ctrl;
    seq_no = (sc&IEEE80211_SCTL_SEQ)>>4;
  }

    // printk("%s openwifi_tx: rate&try: %d %d %03x; %d %d %03x; %d %d %03x; %d %d %03x\n", sdr_compatible_str,
    //   info->status.rates[0].idx,info->status.rates[0].count,info->status.rates[0].flags,
    //   info->status.rates[1].idx,info->status.rates[1].count,info->status.rates[1].flags,
    //   info->status.rates[2].idx,info->status.rates[2].count,info->status.rates[2].flags,
    //   info->status.rates[3].idx,info->status.rates[3].count,info->status.rates[3].flags);

  // -----------end of preprocess some info from header and skb----------------

  // /* HW will perform RTS-CTS when only RTS flags is set.
  //  * HW will perform CTS-to-self when both RTS and CTS flags are set.
  //  * RTS rate and RTS duration will be used also for CTS-to-self.
  //  */
  // if (rc_flags & IEEE80211_TX_RC_USE_RTS_CTS) {
  //   tx_flags |= ieee80211_get_rts_cts_rate(dev, info)->hw_value << 19;
  //   rts_duration = ieee80211_rts_duration(dev, priv->vif[0], // assume all vif have the same config
  //           len_mpdu, info);
  //   printk("%s openwifi_tx: rc_flags & IEEE80211_TX_RC_USE_RTS_CTS\n", sdr_compatible_str);
  // } else if (rc_flags & IEEE80211_TX_RC_USE_CTS_PROTECT) {
  //   tx_flags |= ieee80211_get_rts_cts_rate(dev, info)->hw_value << 19;
  //   rts_duration = ieee80211_ctstoself_duration(dev, priv->vif[0], // assume all vif have the same config
  //           len_mpdu, info);
  //   printk("%s openwifi_tx: rc_flags & IEEE80211_TX_RC_USE_CTS_PROTECT\n", sdr_compatible_str);
  // }

  if(use_ht_aggr)
  {
    if(ieee80211_is_data_qos(frame_control) == false)
    {
      printk("%s openwifi_tx: WARNING packet is not QoS packet!\n", sdr_compatible_str);
      goto openwifi_tx_early_out;
    }

    // psdu = [ MPDU DEL | MPDU | CRC | MPDU padding ]
    len_mpdu_delim_pad = ((len_mpdu + LEN_PHY_CRC)%4 == 0) ? 0 :(4 - (len_mpdu + LEN_PHY_CRC)%4);
    len_psdu = LEN_MPDU_DELIM + len_mpdu + LEN_PHY_CRC + len_mpdu_delim_pad;

    if( (addr1_low32 != addr1_low32_prev) || (addr1_high16 != addr1_high16_prev) || (duration_id != duration_id_prev) || 
      (rate_hw_value != rate_hw_value_prev) || (use_short_gi != use_short_gi_prev) || 
      (prio != prio_prev) || (retry_limit_raw != retry_limit_raw_prev) || (pkt_need_ack != pkt_need_ack_prev) )
    {
      addr1_low32_prev = addr1_low32;
      addr1_high16_prev = addr1_high16;
      duration_id_prev = duration_id;
      rate_hw_value_prev = rate_hw_value;
      use_short_gi_prev = use_short_gi;
      prio_prev = prio;
      retry_limit_raw_prev = retry_limit_raw;
      pkt_need_ack_prev = pkt_need_ack;

      ht_aggr_start = true;
    }
  }
  else
  {
    // psdu = [ MPDU ]
    len_psdu = len_mpdu;
    
    // // Don't need to reset _prev variables every time when it is not ht aggr qos data. Reason:
    // // 1. In 99.9999% cases, the ht always use qos data and goes to prio/queue_idx 2. By not resetting the variable to -1, we can have continuous aggregation packet operation in FPGA queue 2.
    // // 2. In other words, the aggregation operation for queue 2 in FPGA won't be interrupted by other non aggregation packets (control/management/beacon/etc.) that go to queue 0 (or other queues than 2).
    // // 3. From wired domain and upper level ( DSCP, AC (0~3), WMM management, 802.11D service classes and user priority (UP) ) to chip/FPGA queue index, thre should be some (complicated) mapping relationship.
    // // 4. More decent design is setting these aggregation flags (ht_aggr_start) per queue/prio here in driver. But since now only queue 2 and 0 are used (data goes to queue 2, others go to queue 0) in normal (most) cases, let's not go to the decent (complicated) solution immediately.
    // addr1_low32_prev = -1;
    // addr1_high16_prev = -1;
    // duration_id_prev = -1;
    // use_short_gi_prev = -1;
    // rate_hw_value_prev = -1;
    // prio_prev = -1;
    // retry_limit_raw_prev = -1;
    // pkt_need_ack_prev = -1;
  }
  num_dma_symbol = (len_psdu>>TX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS) + ((len_psdu&(TX_INTF_NUM_BYTE_PER_DMA_SYMBOL-1))!=0);

  if ( ( (!pkt_need_ack)&&(priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG]&DMESG_LOG_BROADCAST) ) || ( (pkt_need_ack)&&(priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG]&DMESG_LOG_UNICAST) ) ) 
    printk("%s openwifi_tx: %dB RC%x %dM FC%04x DI%04x ADDR%04x%08x/%04x%08x/%04x%08x flag%08x QoS%02x SC%d_%d retr%d ack%d prio%d q%d wr%d rd%d\n", sdr_compatible_str,
      len_mpdu, rc_flags, (use_ht_rate == false ? wifi_rate_all[rate_hw_value] : wifi_rate_all[rate_hw_value + 12]),frame_control,duration_id, 
      reverse16(addr1_high16), reverse32(addr1_low32), reverse16(addr2_high16), reverse32(addr2_low32), reverse16(addr3_high16), reverse32(addr3_low32),
      info->flags, qos_hdr, seq_no, drv_seqno, retry_limit_raw, pkt_need_ack, prio, drv_ring_idx,
      // use_rts_cts,use_cts_protect|force_use_cts_protect,wifi_rate_all[cts_rate_hw_value],cts_duration,
      ring->bd_wr_idx,ring->bd_rd_idx);

  // check whether the packet is bigger than DMA buffer size
  num_dma_byte = (num_dma_symbol<<TX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS);
  if (num_dma_byte > TX_BD_BUF_SIZE) {
    printk("%s openwifi_tx: WARNING sn %d num_dma_byte > TX_BD_BUF_SIZE\n", sdr_compatible_str, ring->bd_wr_idx);
    goto openwifi_tx_early_out;
  }

  // Copy MPDU delimiter and padding into sk_buff
  if(use_ht_aggr)
  {
    // when skb does not have enough headroom, skb_push will cause kernel panic. headroom needs to be extended if necessary
    if (skb_headroom(skb)<LEN_MPDU_DELIM) {// in case original skb headroom is not enough to host MPDU delimiter
      printk("%s openwifi_tx: WARNING(AGGR) sn %d skb_headroom(skb) %d < LEN_MPDU_DELIM %d\n", sdr_compatible_str, ring->bd_wr_idx, skb_headroom(skb), LEN_MPDU_DELIM);
      if ((skb_new = skb_realloc_headroom(skb, LEN_MPDU_DELIM)) == NULL) {
        printk("%s openwifi_tx: WARNING sn %d skb_realloc_headroom failed!\n", sdr_compatible_str, ring->bd_wr_idx);
        goto openwifi_tx_early_out;
      }
      if (skb->sk != NULL)
        skb_set_owner_w(skb_new, skb->sk);
      dev_kfree_skb(skb);
      skb = skb_new;
    }
    skb_push( skb, LEN_MPDU_DELIM );
    dma_buf = skb->data;

    // fill in MPDU delimiter
    *((u16*)(dma_buf+0)) = ((u16)(len_mpdu+LEN_PHY_CRC) << 4) & 0xFFF0;
    *((u8 *)(dma_buf+2)) = gen_mpdu_delim_crc(*((u16 *)dma_buf));
    *((u8 *)(dma_buf+3)) = 0x4e;

    // Extend sk_buff to hold CRC + MPDU padding + empty MPDU delimiter
    num_byte_pad = num_dma_byte - (LEN_MPDU_DELIM + len_mpdu);
    if (skb_tailroom(skb)<num_byte_pad) {// in case original skb tailroom is not enough to host num_byte_pad
      printk("%s openwifi_tx: WARNING(AGGR) sn %d skb_tailroom(skb) %d < num_byte_pad %d!\n", sdr_compatible_str, ring->bd_wr_idx, skb_tailroom(skb), num_byte_pad);
      if ((skb_new = skb_copy_expand(skb, skb_headroom(skb), num_byte_pad, GFP_KERNEL)) == NULL) {
        printk("%s openwifi_tx: WARNING(AGGR) sn %d skb_copy_expand failed!\n", sdr_compatible_str, ring->bd_wr_idx);
        goto openwifi_tx_early_out;
      }
      if (skb->sk != NULL)
        skb_set_owner_w(skb_new, skb->sk);
      dev_kfree_skb(skb);
      skb = skb_new;
    }
    skb_put( skb, num_byte_pad );

    // fill in MPDU CRC
    *((u32*)(dma_buf+LEN_MPDU_DELIM+len_mpdu)) = gen_mpdu_crc(dma_buf+LEN_MPDU_DELIM, len_mpdu);

    // fill in MPDU delimiter padding
    memset(dma_buf+LEN_MPDU_DELIM+len_mpdu+LEN_PHY_CRC, 0, len_mpdu_delim_pad);

    // num_dma_byte is on 8-byte boundary and len_psdu is on 4 byte boundary.
    // If they have different lengths, add "empty MPDU delimiter" for alignment
    if(num_dma_byte == len_psdu + 4)
    {
      *((u32*)(dma_buf+len_psdu)) = 0x4e140000;
      len_psdu = num_dma_byte;
    }
  }
  else
  {
    // Extend sk_buff to hold padding
    num_byte_pad = num_dma_byte - len_mpdu;
    if (skb_tailroom(skb)<num_byte_pad) {// in case original skb tailroom is not enough to host num_byte_pad
      printk("%s openwifi_tx: WARNING sn %d skb_tailroom(skb) %d < num_byte_pad %d!\n", sdr_compatible_str, ring->bd_wr_idx, skb_tailroom(skb), num_byte_pad);
      if ((skb_new = skb_copy_expand(skb, skb_headroom(skb), num_byte_pad, GFP_KERNEL)) == NULL) {
        printk("%s openwifi_tx: WARNING sn %d skb_copy_expand failed!\n", sdr_compatible_str, ring->bd_wr_idx);
        goto openwifi_tx_early_out;
      }
      if (skb->sk != NULL)
        skb_set_owner_w(skb_new, skb->sk);
      dev_kfree_skb(skb);
      skb = skb_new;
    }
    skb_put( skb, num_byte_pad );

    dma_buf = skb->data;
  }
//  for(i = 0; i <= num_dma_symbol; i++)
//    printk("%16llx\n", (*(u64*)(&(dma_buf[i*8]))));

  retry_limit_hw_value = ( retry_limit_raw==0?0:((retry_limit_raw - 1)&0xF) );

  queue_idx = drv_ring_idx; // from driver ring idx to FPGA queue_idx mapping

  if (use_cts_protect || force_use_cts_protect) {
    rate_signal_value = (use_ht_rate ? rate_hw_value : wifi_mcs_table_11b_force_up[rate_hw_value]);
    cts_rate_signal_value = wifi_mcs_table_11b_force_up[cts_rate_hw_value];
    cts_reg = ((use_cts_protect|force_use_cts_protect)<<31 | cts_use_traffic_rate<<30 | cts_duration<<8 | cts_rate_signal_value<<4 | rate_signal_value);
  }
  phy_hdr_config = ( ht_aggr_start<<20 | rate_hw_value<<16 | use_ht_rate<<15 | use_short_gi<<14 | use_ht_aggr<<13 | len_psdu );
  tx_config = ( prio<<26 | ring->bd_wr_idx<<20 | queue_idx<<18 | retry_limit_hw_value<<14 | pkt_need_ack<<13 | num_dma_symbol );

  /* We must be sure that tx_flags is written last because the HW
   * looks at it to check if the rest of data is valid or not
   */
  //wmb();
  // entry->flags = cpu_to_le32(tx_flags);
  /* We must be sure this has been written before following HW
   * register write, because this write will make the HW attempts
   * to DMA the just-written data
   */
  //wmb();

  spin_lock_irqsave(&priv->lock, flags); // from now on, we'd better avoid interrupt because ring->stop_flag is shared with interrupt

  // -------------check whether FPGA dma fifo and queue (queue_idx) has enough room-------------
  dma_fifo_no_room_flag = tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read();
  hw_queue_len = tx_intf_api->TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read();
  if ( ((dma_fifo_no_room_flag>>queue_idx)&1) || ((NUM_TX_BD-((hw_queue_len>>(queue_idx*8))&0xFF))<=RING_ROOM_THRESHOLD)  || ring->stop_flag>=0 ) {
    if( priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG]&DMESG_LOG_NORMAL_QUEUE_STOP )
      printk("%s openwifi_tx: WARNING ieee80211_stop_queue prio%d queue%d no room flag%x hwq len%08x request%d wr%d rd%d\n", sdr_compatible_str,
              prio, queue_idx, dma_fifo_no_room_flag, hw_queue_len, num_dma_symbol, ring->bd_wr_idx, ring->bd_rd_idx);

    ieee80211_stop_queue(dev, prio); // here we should stop those prio related to the queue idx flag set in TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read
    ring->stop_flag = prio;
    if (priv->stat.stat_enable) {
      priv->stat.tx_prio_stop1_num[prio]++;
      priv->stat.tx_queue_stop1_num[queue_idx]++;
    }
    // goto openwifi_tx_early_out_after_lock;
  }
  // --------end of check whether FPGA fifo (queue_idx) has enough room------------

  status = dma_async_is_tx_complete(priv->tx_chan, priv->tx_cookie, NULL, NULL);
  while(delay_count<100 && status!=DMA_COMPLETE) {
    status = dma_async_is_tx_complete(priv->tx_chan, priv->tx_cookie, NULL, NULL);
    delay_count++;
    udelay(4);
    // udelay(priv->stat.dbg_ch1);
  }
  if (status!=DMA_COMPLETE) {
    printk("%s openwifi_tx: WARNING status!=DMA_COMPLETE\n", sdr_compatible_str);
    goto openwifi_tx_early_out_after_lock;
  }

//-------------------------fire skb DMA to hardware----------------------------------
  dma_mapping_addr = dma_map_single(priv->tx_chan->device->dev, dma_buf,
         num_dma_byte, DMA_MEM_TO_DEV);

  if (dma_mapping_error(priv->tx_chan->device->dev,dma_mapping_addr)) {
    // dev_err(priv->tx_chan->device->dev, "sdr,sdr openwifi_tx: WARNING TX DMA mapping error\n");
    printk("%s openwifi_tx: WARNING sn %d TX DMA mapping error\n", sdr_compatible_str, ring->bd_wr_idx);
    goto openwifi_tx_early_out_after_lock;
  }

  sg_init_table(&(priv->tx_sg), 1); // only need to be initialized once in openwifi_start
  sg_dma_address( &(priv->tx_sg) ) = dma_mapping_addr;
  sg_dma_len( &(priv->tx_sg) ) = num_dma_byte;
  
  tx_intf_api->TX_INTF_REG_CTS_TOSELF_CONFIG_write(cts_reg);
  tx_intf_api->TX_INTF_REG_TX_CONFIG_write(tx_config);
  tx_intf_api->TX_INTF_REG_PHY_HDR_CONFIG_write(phy_hdr_config);
  priv->txd = priv->tx_chan->device->device_prep_slave_sg(priv->tx_chan, &(priv->tx_sg),1,DMA_MEM_TO_DEV, DMA_CTRL_ACK | DMA_PREP_INTERRUPT, NULL);
  if (!(priv->txd)) {
    printk("%s openwifi_tx: WARNING sn %d device_prep_slave_sg %p\n", sdr_compatible_str, ring->bd_wr_idx, (void*)(priv->txd));
    goto openwifi_tx_after_dma_mapping;
  }

  priv->tx_cookie = priv->txd->tx_submit(priv->txd);

  if (dma_submit_error(priv->tx_cookie)) {
    printk("%s openwifi_tx: WARNING sn %d dma_submit_error(tx_cookie) %d\n", sdr_compatible_str, ring->bd_wr_idx, (u32)(priv->tx_cookie));
    goto openwifi_tx_after_dma_mapping;
  }

  // seems everything is ok. let's mark this pkt in bd descriptor ring
  ring->bds[ring->bd_wr_idx].prio = prio;
  ring->bds[ring->bd_wr_idx].len_mpdu = len_mpdu;
  ring->bds[ring->bd_wr_idx].seq_no = seq_no;
  ring->bds[ring->bd_wr_idx].skb_linked = skb;
  ring->bds[ring->bd_wr_idx].dma_mapping_addr = dma_mapping_addr;

  ring->bd_wr_idx = ((ring->bd_wr_idx+1)&(NUM_TX_BD-1));

  dma_async_issue_pending(priv->tx_chan);

  spin_unlock_irqrestore(&priv->lock, flags);

  if (priv->stat.stat_enable) {
    priv->stat.tx_prio_num[prio]++;
    priv->stat.tx_queue_num[queue_idx]++;
  }

  return;

openwifi_tx_after_dma_mapping:
  dma_unmap_single(priv->tx_chan->device->dev, dma_mapping_addr, num_dma_byte, DMA_MEM_TO_DEV);

openwifi_tx_early_out_after_lock:
  spin_unlock_irqrestore(&priv->lock, flags);
  report_pkt_loss_due_to_driver_drop(dev, skb);
  // dev_kfree_skb(skb);
  // printk("%s openwifi_tx: WARNING openwifi_tx_after_dma_mapping phy_tx_sn %d queue %d\n", sdr_compatible_str,priv->phy_tx_sn,queue_idx);
  return;

openwifi_tx_early_out:
  report_pkt_loss_due_to_driver_drop(dev, skb);
  // dev_kfree_skb(skb);
  // printk("%s openwifi_tx: WARNING openwifi_tx_early_out phy_tx_sn %d queue %d\n", sdr_compatible_str,priv->phy_tx_sn,queue_idx);
}

static int openwifi_set_antenna(struct ieee80211_hw *dev, u32 tx_ant, u32 rx_ant)
{
  struct openwifi_priv *priv = dev->priv;
  u8 fpga_tx_ant_setting, target_rx_ant;
  u32 atten_mdb_tx0, atten_mdb_tx1;
  struct ctrl_outs_control ctrl_out;
  int ret;
  
  printk("%s openwifi_set_antenna: tx_ant%d rx_ant%d\n",sdr_compatible_str,tx_ant,rx_ant);

  if (tx_ant >= 4 || tx_ant == 0) {
    return -EINVAL;
  } else if (rx_ant >= 3 || rx_ant == 0) {
    return -EINVAL;
  }

  fpga_tx_ant_setting = ((tx_ant<=2)?(tx_ant):(tx_ant+16));
  target_rx_ant = ((rx_ant&1)?0:1);

  // try rf chip setting firstly, only update internal state variable when rf chip succeed
  atten_mdb_tx0 = ((tx_ant&1)?(AD9361_RADIO_ON_TX_ATT+priv->rf_reg_val[RF_TX_REG_IDX_ATT]):AD9361_RADIO_OFF_TX_ATT);
  atten_mdb_tx1 = ((tx_ant&2)?(AD9361_RADIO_ON_TX_ATT+priv->rf_reg_val[RF_TX_REG_IDX_ATT]):AD9361_RADIO_OFF_TX_ATT);
  ret = ad9361_set_tx_atten(priv->ad9361_phy, atten_mdb_tx0, true, false, true);
  if (ret < 0) {
    printk("%s openwifi_set_antenna: WARNING ad9361_set_tx_atten ant0 %d FAIL!\n",sdr_compatible_str, atten_mdb_tx0);
    return -EINVAL;
  } else {
    printk("%s openwifi_set_antenna: ad9361_set_tx_atten ant0 %d OK\n",sdr_compatible_str, atten_mdb_tx0);
  }
  ret = ad9361_set_tx_atten(priv->ad9361_phy, atten_mdb_tx1, false, true, true);
  if (ret < 0) {
    printk("%s openwifi_set_antenna: WARNING ad9361_set_tx_atten ant1 %d FAIL!\n",sdr_compatible_str, atten_mdb_tx1);
    return -EINVAL;
  } else {
    printk("%s openwifi_set_antenna: ad9361_set_tx_atten ant1 %d OK\n",sdr_compatible_str, atten_mdb_tx1);
  }

  ctrl_out.en_mask = priv->ctrl_out.en_mask;
  ctrl_out.index = (target_rx_ant==0?AD9361_CTRL_OUT_INDEX_ANT0:AD9361_CTRL_OUT_INDEX_ANT1);
  ret = ad9361_ctrl_outs_setup(priv->ad9361_phy, &(ctrl_out));
  if (ret < 0) {
    printk("%s openwifi_set_antenna: WARNING ad9361_ctrl_outs_setup en_mask 0x%02x index 0x%02x FAIL!\n",sdr_compatible_str, ctrl_out.en_mask, ctrl_out.index);
    return -EINVAL;
  } else {
    printk("%s openwifi_set_antenna: ad9361_ctrl_outs_setup en_mask 0x%02x index 0x%02x\n",sdr_compatible_str, ctrl_out.en_mask, ctrl_out.index);
  }

  tx_intf_api->TX_INTF_REG_ANT_SEL_write(fpga_tx_ant_setting);
  ret = tx_intf_api->TX_INTF_REG_ANT_SEL_read();
  if (ret != fpga_tx_ant_setting) {
    printk("%s openwifi_set_antenna: WARNING TX_INTF_REG_ANT_SEL_write target %d read back %d\n",sdr_compatible_str, fpga_tx_ant_setting, ret);
    return -EINVAL;
  } else {
    printk("%s openwifi_set_antenna: TX_INTF_REG_ANT_SEL_write value %d\n",sdr_compatible_str, ret);
  }

  rx_intf_api->RX_INTF_REG_ANT_SEL_write(target_rx_ant);
  ret = rx_intf_api->RX_INTF_REG_ANT_SEL_read();
  if (ret != target_rx_ant) {
    printk("%s openwifi_set_antenna: WARNING RX_INTF_REG_ANT_SEL_write target %d read back %d\n",sdr_compatible_str, target_rx_ant, ret);
    return -EINVAL;
  } else {
    printk("%s openwifi_set_antenna: RX_INTF_REG_ANT_SEL_write value %d\n",sdr_compatible_str, ret);
  }

  // update internal state variable
  priv->runtime_tx_ant_cfg = tx_ant;
  priv->runtime_rx_ant_cfg = rx_ant;

  if (TX_OFFSET_TUNING_ENABLE)
    priv->tx_intf_cfg = ((tx_ant&1)?TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0:TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1);//NO USE
  else {
    if (tx_ant == 3)
      priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_0MHZ_ANT_BOTH;
    else
      priv->tx_intf_cfg = ((tx_ant&1)?TX_INTF_BW_20MHZ_AT_0MHZ_ANT0:TX_INTF_BW_20MHZ_AT_0MHZ_ANT1);
  }

  priv->rx_intf_cfg = (target_rx_ant==0?RX_INTF_BW_20MHZ_AT_0MHZ_ANT0:RX_INTF_BW_20MHZ_AT_0MHZ_ANT1);
  priv->ctrl_out.index=ctrl_out.index;

  priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];
  priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];

  return 0;
}
static int openwifi_get_antenna(struct ieee80211_hw *dev, u32 *tx_ant, u32 *rx_ant)
{
  struct openwifi_priv *priv = dev->priv;

  *tx_ant = priv->runtime_tx_ant_cfg;
  *rx_ant = priv->runtime_rx_ant_cfg;

  printk("%s openwifi_get_antenna: tx_ant%d rx_ant%d\n",sdr_compatible_str, *tx_ant, *rx_ant);

  printk("%s openwifi_get_antenna: drv tx cfg %d offset %d drv rx cfg %d offset %d drv ctrl_out sel %x\n",sdr_compatible_str,
  priv->tx_intf_cfg, priv->tx_freq_offset_to_lo_MHz, priv->rx_intf_cfg, priv->rx_freq_offset_to_lo_MHz, priv->ctrl_out.index);
  
  printk("%s openwifi_get_antenna: fpga tx sel %d rx sel %d\n", sdr_compatible_str, 
  tx_intf_api->TX_INTF_REG_ANT_SEL_read(), rx_intf_api->RX_INTF_REG_ANT_SEL_read());
  
  printk("%s openwifi_get_antenna: rf tx att0 %d tx att1 %d ctrl_out sel %x\n", sdr_compatible_str, 
  ad9361_get_tx_atten(priv->ad9361_phy, 1), ad9361_get_tx_atten(priv->ad9361_phy, 2), ad9361_spi_read(priv->ad9361_phy->spi, REG_CTRL_OUTPUT_POINTER));

  return 0;
}

static int openwifi_start(struct ieee80211_hw *dev)
{
  struct openwifi_priv *priv = dev->priv;
  int ret, i;
  u32 reg;

  for (i=0; i<MAX_NUM_VIF; i++) {
    priv->vif[i] = NULL;
  }

  // //keep software registers persistent between NIC down and up for multiple times
  /*memset(priv->drv_tx_reg_val, 0, sizeof(priv->drv_tx_reg_val));
  memset(priv->drv_rx_reg_val, 0, sizeof(priv->drv_rx_reg_val));
  memset(priv->drv_xpu_reg_val, 0, sizeof(priv->drv_xpu_reg_val));
  memset(priv->rf_reg_val,0,sizeof(priv->rf_reg_val));
  priv->drv_xpu_reg_val[DRV_XPU_REG_IDX_GIT_REV] = GIT_REV;*/

  //turn on radio
  openwifi_set_antenna(dev, priv->runtime_tx_ant_cfg, priv->runtime_rx_ant_cfg);
  reg = ad9361_get_tx_atten(priv->ad9361_phy, ((priv->runtime_tx_ant_cfg==1 || priv->runtime_tx_ant_cfg==3)?1:2));
  if (reg == (AD9361_RADIO_ON_TX_ATT+priv->rf_reg_val[RF_TX_REG_IDX_ATT])) {
    priv->rfkill_off = 1;// 0 off, 1 on
    printk("%s openwifi_start: rfkill radio on\n",sdr_compatible_str);
  }
  else
    printk("%s openwifi_start: WARNING rfkill radio on failed. tx att read %d require %d\n",sdr_compatible_str, reg, AD9361_RADIO_ON_TX_ATT+priv->rf_reg_val[RF_TX_REG_IDX_ATT]);

  rx_intf_api->hw_init(priv->rx_intf_cfg,8,8);
  tx_intf_api->hw_init(priv->tx_intf_cfg,8,8,priv->fpga_type);
  openofdm_tx_api->hw_init(priv->openofdm_tx_cfg);
  openofdm_rx_api->hw_init(priv->openofdm_rx_cfg);
  xpu_api->hw_init(priv->xpu_cfg);

  xpu_api->XPU_REG_MAC_ADDR_write(priv->mac_addr);

  printk("%s openwifi_start: rx_intf_cfg %d openofdm_rx_cfg %d tx_intf_cfg %d openofdm_tx_cfg %d\n",sdr_compatible_str, priv->rx_intf_cfg, priv->openofdm_rx_cfg, priv->tx_intf_cfg, priv->openofdm_tx_cfg);
  printk("%s openwifi_start: rx_freq_offset_to_lo_MHz %d tx_freq_offset_to_lo_MHz %d\n",sdr_compatible_str, priv->rx_freq_offset_to_lo_MHz, priv->tx_freq_offset_to_lo_MHz);

  tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x30004); //disable tx interrupt
  rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_write(0x100); // disable rx interrupt by interrupt test mode
  rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(1); // hold M AXIS in reset status

  // priv->rx_chan = dma_request_slave_channel(&(priv->pdev->dev), "rx_dma_s2mm");
  priv->rx_chan = dma_request_chan(&(priv->pdev->dev), "rx_dma_s2mm");
  if (IS_ERR(priv->rx_chan) || priv->rx_chan==NULL) {
    ret = PTR_ERR(priv->rx_chan);
    if (ret != -EPROBE_DEFER) {
      pr_err("%s openwifi_start: No Rx channel ret %d priv->rx_chan 0x%p\n",sdr_compatible_str, ret, priv->rx_chan);
      goto err_dma;
    }
  }

  // priv->tx_chan = dma_request_slave_channel(&(priv->pdev->dev), "tx_dma_mm2s");
  priv->tx_chan = dma_request_chan(&(priv->pdev->dev), "tx_dma_mm2s");
  if (IS_ERR(priv->tx_chan) || priv->tx_chan==NULL) {
    ret = PTR_ERR(priv->tx_chan);
    if (ret != -EPROBE_DEFER) {
      pr_err("%s openwifi_start: No Tx channel ret %d priv->tx_chan 0x%p\n",sdr_compatible_str, ret, priv->tx_chan);
      goto err_dma;
    }
  }
  printk("%s openwifi_start: DMA channel setup successfully. priv->rx_chan 0x%p priv->tx_chan 0x%p\n",sdr_compatible_str, priv->rx_chan, priv->tx_chan);

  ret = openwifi_init_rx_ring(priv);
  if (ret) {
    printk("%s openwifi_start: openwifi_init_rx_ring ret %d\n", sdr_compatible_str,ret);
    goto err_free_rings;
  }

  priv->seqno=0;
  for (i=0; i<MAX_NUM_SW_QUEUE; i++) {
    if ((ret = openwifi_init_tx_ring(priv, i))) {
      printk("%s openwifi_start: openwifi_init_tx_ring %d ret %d\n", sdr_compatible_str, i, ret);
      goto err_free_rings;
    }
  }

  if ( (ret = rx_dma_setup(dev)) ) {
    printk("%s openwifi_start: rx_dma_setup ret %d\n", sdr_compatible_str,ret);
    goto err_free_rings;
  }

  priv->irq_rx = irq_of_parse_and_map(priv->pdev->dev.of_node, 1);
  ret = request_irq(priv->irq_rx, openwifi_rx_interrupt,
      IRQF_SHARED, "sdr,rx_pkt_intr", dev);
  if (ret) {
    wiphy_err(dev->wiphy, "openwifi_start:failed to register IRQ handler openwifi_rx_interrupt\n");
    goto err_free_rings;
  } else {
    printk("%s openwifi_start: irq_rx %d\n", sdr_compatible_str, priv->irq_rx);
  }

  priv->irq_tx = irq_of_parse_and_map(priv->pdev->dev.of_node, 3);
  ret = request_irq(priv->irq_tx, openwifi_tx_interrupt,
      IRQF_SHARED, "sdr,tx_itrpt", dev);
  if (ret) {
    wiphy_err(dev->wiphy, "openwifi_start: failed to register IRQ handler openwifi_tx_interrupt\n");
    goto err_free_rings;
  } else {
    printk("%s openwifi_start: irq_tx %d\n", sdr_compatible_str, priv->irq_tx);
  }

  rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_write(0x000); // enable rx interrupt get normal fcs valid pass through ddc to ARM
  tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x4); //enable tx interrupt
  rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(0); // release M AXIS
  xpu_api->XPU_REG_TSF_LOAD_VAL_write(0,0); // reset tsf timer

  priv->stat.csma_cfg0 = xpu_api->XPU_REG_FORCE_IDLE_MISC_read();

  // disable ad9361 auto calibration and enable openwifi fpga spi control
  priv->ad9361_phy->state->auto_cal_en = false;   // turn off auto Tx quadrature calib.
  priv->ad9361_phy->state->manual_tx_quad_cal_en = true;  // turn on manual Tx quadrature calib.
  xpu_api->XPU_REG_SPI_DISABLE_write(0);

// normal_out:
  printk("%s openwifi_start: normal end\n", sdr_compatible_str);
  return 0;

err_free_rings:
  openwifi_free_rx_ring(priv);
  for (i=0; i<MAX_NUM_SW_QUEUE; i++)
    openwifi_free_tx_ring(priv, i);

err_dma:
  ret = -1;
  printk("%s openwifi_start: abnormal end ret %d\n", sdr_compatible_str, ret);
  return ret;
}

static void openwifi_stop(struct ieee80211_hw *dev)
{
  struct openwifi_priv *priv = dev->priv;
  u32 reg, reg1;
  int i;

  // enable ad9361 auto calibration and disable openwifi fpga spi control
  priv->ad9361_phy->state->auto_cal_en = true;   // turn on auto Tx quadrature calib.
  priv->ad9361_phy->state->manual_tx_quad_cal_en = false;  // turn off manual Tx quadrature calib.
  xpu_api->XPU_REG_SPI_DISABLE_write(1);

  //turn off radio
  #if 1
  ad9361_tx_mute(priv->ad9361_phy, 1);
  reg = ad9361_get_tx_atten(priv->ad9361_phy, 2);
  reg1 = ad9361_get_tx_atten(priv->ad9361_phy, 1);
  if (reg == AD9361_RADIO_OFF_TX_ATT && reg1 == AD9361_RADIO_OFF_TX_ATT ) {
    priv->rfkill_off = 0;// 0 off, 1 on
    printk("%s openwifi_stop: rfkill radio off\n",sdr_compatible_str);
  }
  else
    printk("%s openwifi_stop: WARNING rfkill radio off failed. tx att read %d %d require %d\n",sdr_compatible_str, reg, reg1, AD9361_RADIO_OFF_TX_ATT);
  #endif

  //ieee80211_stop_queue(dev, 0);
  tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x30004); //disable tx interrupt
  rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_write(0x100); // disable fcs_valid by interrupt test mode
  rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(1); // hold M AXIS in reset status

  for (i=0; i<MAX_NUM_VIF; i++) {
    priv->vif[i] = NULL;
  }

  openwifi_free_rx_ring(priv);
  for (i=0; i<MAX_NUM_SW_QUEUE; i++)
    openwifi_free_tx_ring(priv, i);

  pr_info("%s openwifi_stop: dropped channel %s\n", sdr_compatible_str, dma_chan_name(priv->rx_chan));
  dmaengine_terminate_all(priv->rx_chan);
  dma_release_channel(priv->rx_chan);
  pr_info("%s openwifi_stop: dropped channel %s\n", sdr_compatible_str, dma_chan_name(priv->tx_chan));
  dmaengine_terminate_all(priv->tx_chan);
  dma_release_channel(priv->tx_chan);

  //priv->rf->stop(dev);

  free_irq(priv->irq_rx, dev);
  free_irq(priv->irq_tx, dev);

// normal_out:
  printk("%s openwifi_stop\n", sdr_compatible_str);
}

static u64 openwifi_get_tsf(struct ieee80211_hw *dev,
         struct ieee80211_vif *vif)
{
  u32 tsft_low, tsft_high;

  tsft_low = xpu_api->XPU_REG_TSF_RUNTIME_VAL_LOW_read();
  tsft_high = xpu_api->XPU_REG_TSF_RUNTIME_VAL_HIGH_read();
  //printk("%s openwifi_get_tsf: %08x%08x\n", sdr_compatible_str,tsft_high,tsft_low);
  return( ( (u64)tsft_low ) | ( ((u64)tsft_high)<<32 ) );
}

static void openwifi_set_tsf(struct ieee80211_hw *hw, struct ieee80211_vif *vif, u64 tsf)
{
  u32 tsft_high = ((tsf >> 32)&0xffffffff);
  u32 tsft_low  = (tsf&0xffffffff);
  xpu_api->XPU_REG_TSF_LOAD_VAL_write(tsft_high,tsft_low);
  printk("%s openwifi_set_tsf: %08x%08x\n", sdr_compatible_str,tsft_high,tsft_low);
}

static void openwifi_reset_tsf(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
  xpu_api->XPU_REG_TSF_LOAD_VAL_write(0,0);
  printk("%s openwifi_reset_tsf\n", sdr_compatible_str);
}

static int openwifi_set_rts_threshold(struct ieee80211_hw *hw, u32 value)
{
  printk("%s openwifi_set_rts_threshold WARNING value %d\n", sdr_compatible_str,value);
  return(0);
}

static void openwifi_beacon_work(struct work_struct *work)
{
  struct openwifi_vif *vif_priv =
    container_of(work, struct openwifi_vif, beacon_work.work);
  struct ieee80211_vif *vif =
    container_of((void *)vif_priv, struct ieee80211_vif, drv_priv);
  struct ieee80211_hw *dev = vif_priv->dev;
  struct ieee80211_mgmt *mgmt;
  struct sk_buff *skb;

  /* don't overflow the tx ring */
  if (ieee80211_queue_stopped(dev, 0))
    goto resched;

  /* grab a fresh beacon */
  skb = ieee80211_beacon_get(dev, vif);
  if (!skb)
    goto resched;

  /*
   * update beacon timestamp w/ TSF value
   * TODO: make hardware update beacon timestamp
   */
  mgmt = (struct ieee80211_mgmt *)skb->data;
  mgmt->u.beacon.timestamp = cpu_to_le64(openwifi_get_tsf(dev, vif));

  /* TODO: use actual beacon queue */
  skb_set_queue_mapping(skb, 0);
  openwifi_tx(dev, NULL, skb);

resched:
  /*
   * schedule next beacon
   * TODO: use hardware support for beacon timing
   */
  schedule_delayed_work(&vif_priv->beacon_work, usecs_to_jiffies(1024 * vif->bss_conf.beacon_int));
  // printk("%s openwifi_beacon_work beacon_int %d\n", sdr_compatible_str, vif->bss_conf.beacon_int);
}

static int openwifi_add_interface(struct ieee80211_hw *dev,
         struct ieee80211_vif *vif)
{
  int i;
  struct openwifi_priv *priv = dev->priv;
  struct openwifi_vif *vif_priv;

  switch (vif->type) {
  case NL80211_IFTYPE_AP:
  case NL80211_IFTYPE_STATION:
  case NL80211_IFTYPE_ADHOC:
  case NL80211_IFTYPE_MONITOR:
  case NL80211_IFTYPE_MESH_POINT:
    break;
  default:
    return -EOPNOTSUPP;
  }
  // let's support more than 1 interface
  for (i=0; i<MAX_NUM_VIF; i++) {
    if (priv->vif[i] == NULL)
      break;
  }

  printk("%s openwifi_add_interface start. vif for loop result %d\n", sdr_compatible_str, i);

  if (i==MAX_NUM_VIF)
    return -EBUSY;

  priv->vif[i] = vif;
  
  /* Initialize driver private area */
  vif_priv = (struct openwifi_vif *)&vif->drv_priv;
  vif_priv->idx = i;

  vif_priv->dev = dev;
  INIT_DELAYED_WORK(&vif_priv->beacon_work, openwifi_beacon_work);
  vif_priv->enable_beacon = false;

  priv->mac_addr[0] = vif->addr[0];
  priv->mac_addr[1] = vif->addr[1];
  priv->mac_addr[2] = vif->addr[2];
  priv->mac_addr[3] = vif->addr[3];
  priv->mac_addr[4] = vif->addr[4];
  priv->mac_addr[5] = vif->addr[5];
  xpu_api->XPU_REG_MAC_ADDR_write(priv->mac_addr); // set mac addr in fpga

  printk("%s openwifi_add_interface end with vif idx %d addr %02x:%02x:%02x:%02x:%02x:%02x\n", sdr_compatible_str,vif_priv->idx,
  vif->addr[0],vif->addr[1],vif->addr[2],vif->addr[3],vif->addr[4],vif->addr[5]);

  return 0;
}

static void openwifi_remove_interface(struct ieee80211_hw *dev,
             struct ieee80211_vif *vif)
{
  struct openwifi_vif *vif_priv;
  struct openwifi_priv *priv = dev->priv;
  
  vif_priv = (struct openwifi_vif *)&vif->drv_priv;
  priv->vif[vif_priv->idx] = NULL;
  printk("%s openwifi_remove_interface vif idx %d\n", sdr_compatible_str, vif_priv->idx);
}

static int openwifi_config(struct ieee80211_hw *dev, u32 changed)
{
  struct openwifi_priv *priv = dev->priv;
  struct ieee80211_conf *conf = &dev->conf;
  static struct ieee80211_conf channel_conf_tmp;
  static struct ieee80211_channel channel_tmp;

  channel_conf_tmp.chandef.chan = (&channel_tmp);

  if (changed & IEEE80211_CONF_CHANGE_CHANNEL) {
    if ( priv->stat.restrict_freq_mhz>0 && (conf->chandef.chan->center_freq != priv->stat.restrict_freq_mhz) ) {
      printk("%s openwifi_config avoid Linux requested freq %dMHz (restrict freq %dMHz)\n", sdr_compatible_str, 
      conf->chandef.chan->center_freq, priv->stat.restrict_freq_mhz);

      channel_conf_tmp.chandef.chan->center_freq = priv->stat.restrict_freq_mhz;
      priv->rf->set_chan(dev, &channel_conf_tmp);
    } else {
      priv->rf->set_chan(dev, conf);
    }
  } else
    printk("%s openwifi_config changed flag %08x\n", sdr_compatible_str, changed);
    
  return 0;
}

static void openwifi_bss_info_changed(struct ieee80211_hw *dev,
             struct ieee80211_vif *vif,
             struct ieee80211_bss_conf *info,
             u32 changed)
{
  struct openwifi_priv *priv = dev->priv;
  struct openwifi_vif *vif_priv;
  u32 bssid_low, bssid_high;

  vif_priv = (struct openwifi_vif *)&vif->drv_priv;

  //be careful: we don have valid chip, so registers addresses in priv->map->BSSID[0] are not valid! should not print it!
  //printk("%s openwifi_bss_info_changed map bssid %02x%02x%02x%02x%02x%02x\n",sdr_compatible_str,priv->map->BSSID[0],priv->map->BSSID[1],priv->map->BSSID[2],priv->map->BSSID[3],priv->map->BSSID[4],priv->map->BSSID[5]);
  if (changed & BSS_CHANGED_BSSID) {
    printk("%s openwifi_bss_info_changed BSS_CHANGED_BSSID %02x%02x%02x%02x%02x%02x\n",sdr_compatible_str,info->bssid[0],info->bssid[1],info->bssid[2],info->bssid[3],info->bssid[4],info->bssid[5]);
    // write new bssid to our HW, and do not change bssid filter
    //u32 bssid_filter_high = xpu_api->XPU_REG_BSSID_FILTER_HIGH_read();
    bssid_low = ( *( (u32*)(info->bssid) ) );
    bssid_high = ( *( (u16*)(info->bssid+4) ) );

    //bssid_filter_high = (bssid_filter_high&0x80000000);
    //bssid_high = (bssid_high|bssid_filter_high);
    xpu_api->XPU_REG_BSSID_FILTER_LOW_write(bssid_low);
    xpu_api->XPU_REG_BSSID_FILTER_HIGH_write(bssid_high);
  }

  if (changed & BSS_CHANGED_BEACON_INT) {
    printk("%s openwifi_bss_info_changed WARNING BSS_CHANGED_BEACON_INT %x\n",sdr_compatible_str,info->beacon_int);
  }

  if (changed & BSS_CHANGED_TXPOWER)
    printk("%s openwifi_bss_info_changed WARNING BSS_CHANGED_TXPOWER %x\n",sdr_compatible_str,info->txpower);

  if (changed & BSS_CHANGED_ERP_CTS_PROT)
    printk("%s openwifi_bss_info_changed WARNING BSS_CHANGED_ERP_CTS_PROT %x\n",sdr_compatible_str,info->use_cts_prot);

  if (changed & BSS_CHANGED_BASIC_RATES)
    printk("%s openwifi_bss_info_changed WARNING BSS_CHANGED_BASIC_RATES %x\n",sdr_compatible_str,info->basic_rates);

  if (changed & (BSS_CHANGED_ERP_SLOT | BSS_CHANGED_ERP_PREAMBLE)) {
    printk("%s openwifi_bss_info_changed WARNING BSS_CHANGED_ERP_SLOT %d BSS_CHANGED_ERP_PREAMBLE %d short slot %d\n",sdr_compatible_str,
    changed&BSS_CHANGED_ERP_SLOT,changed&BSS_CHANGED_ERP_PREAMBLE,info->use_short_slot);
    if (info->use_short_slot && priv->use_short_slot==false) {
      priv->use_short_slot=true;
      xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16)|priv->actual_rx_lo );
    } else if ((!info->use_short_slot) && priv->use_short_slot==true) {
      priv->use_short_slot=false;
      xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16)|priv->actual_rx_lo );
    }
  }

  if (changed & BSS_CHANGED_BEACON_ENABLED) {
    printk("%s openwifi_bss_info_changed WARNING BSS_CHANGED_BEACON_ENABLED\n",sdr_compatible_str);
    vif_priv->enable_beacon = info->enable_beacon;
  }

  if (changed & (BSS_CHANGED_BEACON_ENABLED | BSS_CHANGED_BEACON)) {
    cancel_delayed_work_sync(&vif_priv->beacon_work);
    if (vif_priv->enable_beacon) {
      schedule_work(&vif_priv->beacon_work.work);
      printk("%s openwifi_bss_info_changed WARNING enable_beacon\n",sdr_compatible_str);
    }
    printk("%s openwifi_bss_info_changed WARNING BSS_CHANGED_BEACON_ENABLED %d BSS_CHANGED_BEACON %d\n",sdr_compatible_str,
    changed&BSS_CHANGED_BEACON_ENABLED,changed&BSS_CHANGED_BEACON);
  }
}
// helper function
u32 log2val(u32 val){
  u32 ret_val = 0 ;
  while(val>1){
    val = val >> 1 ;
    ret_val ++ ;
  }
  return ret_val ;
}

static int openwifi_conf_tx(struct ieee80211_hw *dev, struct ieee80211_vif *vif, u16 queue,
        const struct ieee80211_tx_queue_params *params)
{
  struct openwifi_priv *priv = dev->priv;
  u32 reg_val, cw_min_exp, cw_max_exp; 
  
  if (priv->stat.cw_max_min_cfg == 0) {
    printk("%s openwifi_conf_tx: [queue %d], aifs: %d, cw_min: %d, cw_max: %d, txop: %d, aifs and txop ignored\n",
      sdr_compatible_str,queue,params->aifs,params->cw_min,params->cw_max,params->txop);

    reg_val=xpu_api->XPU_REG_CSMA_CFG_read();
    cw_min_exp = (log2val(params->cw_min + 1) & 0x0F);
    cw_max_exp = (log2val(params->cw_max + 1) & 0x0F);
    switch(queue){
      case 0: reg_val = ( (reg_val & 0xFFFFFF00) | ((cw_min_exp | (cw_max_exp << 4)) << 0) );  break; 
      case 1: reg_val = ( (reg_val & 0xFFFF00FF) | ((cw_min_exp | (cw_max_exp << 4)) << 8) );  break; 
      case 2: reg_val = ( (reg_val & 0xFF00FFFF) | ((cw_min_exp | (cw_max_exp << 4)) << 16) ); break; 
      case 3: reg_val = ( (reg_val & 0x00FFFFFF) | ((cw_min_exp | (cw_max_exp << 4)) << 24) ); break;
      default: printk("%s openwifi_conf_tx: WARNING queue %d does not exist",sdr_compatible_str, queue); return(0);
    }
  } else {
    reg_val = priv->stat.cw_max_min_cfg;
    printk("%s openwifi_conf_tx: override cw max min for q3 to q0: %d %d; %d %d; %d %d; %d %d\n",
      sdr_compatible_str,
      (1<<((reg_val>>28)&0xF))-1, 
      (1<<((reg_val>>24)&0xF))-1, 
      (1<<((reg_val>>20)&0xF))-1, 
      (1<<((reg_val>>16)&0xF))-1, 
      (1<<((reg_val>>12)&0xF))-1, 
      (1<<((reg_val>> 8)&0xF))-1, 
      (1<<((reg_val>> 4)&0xF))-1, 
      (1<<((reg_val>> 0)&0xF))-1);
  }
  xpu_api->XPU_REG_CSMA_CFG_write(reg_val);
  return(0);
}

static u64 openwifi_prepare_multicast(struct ieee80211_hw *dev,
             struct netdev_hw_addr_list *mc_list)
{
  printk("%s openwifi_prepare_multicast\n", sdr_compatible_str);
  return netdev_hw_addr_list_count(mc_list);
}

static void openwifi_configure_filter(struct ieee80211_hw *dev,
             unsigned int changed_flags,
             unsigned int *total_flags,
             u64 multicast)
{
  struct openwifi_priv *priv = dev->priv;
  u32 filter_flag;

  (*total_flags) &= SDR_SUPPORTED_FILTERS;
  (*total_flags) |= FIF_ALLMULTI; //because we need to pass all multicast (no matter it is for us or not) to upper layer

  filter_flag = (*total_flags);

  filter_flag = (filter_flag|UNICAST_FOR_US|BROADCAST_ALL_ONE|BROADCAST_ALL_ZERO);
  //filter_flag = (filter_flag|UNICAST_FOR_US|BROADCAST_ALL_ONE|BROADCAST_ALL_ZERO|MONITOR_ALL); // all pkt will be delivered to arm

  //if (priv->vif[0]->type == NL80211_IFTYPE_MONITOR)
  if ((filter_flag&0xf0) == 0xf0) //FIF_BCN_PRBRESP_PROMISC/FIF_CONTROL/FIF_OTHER_BSS/FIF_PSPOLL are set means monitor mode  
    filter_flag = (filter_flag|MONITOR_ALL);
  else
    filter_flag = (filter_flag&(~MONITOR_ALL));

  if ( !(filter_flag&FIF_BCN_PRBRESP_PROMISC) )
    filter_flag = (filter_flag|MY_BEACON);

  filter_flag = (filter_flag|FIF_PSPOLL);

  if (priv->stat.rx_monitor_all)
    filter_flag = (filter_flag|MONITOR_ALL);

  xpu_api->XPU_REG_FILTER_FLAG_write(filter_flag|HIGH_PRIORITY_DISCARD_FLAG);
  //xpu_api->XPU_REG_FILTER_FLAG_write(filter_flag); //do not discard any pkt

  printk("%s openwifi_configure_filter MON %d M_BCN %d BST0 %d BST1 %d UST %d PB_RQ %d PS_PL %d O_BSS %d CTL %d BCN_PRP %d PCP_FL %d FCS_FL %d ALL_MUT %d\n", sdr_compatible_str, 
  (filter_flag>>13)&1,(filter_flag>>12)&1,(filter_flag>>11)&1,(filter_flag>>10)&1,(filter_flag>>9)&1,(filter_flag>>8)&1,(filter_flag>>7)&1,(filter_flag>>6)&1,(filter_flag>>5)&1,(filter_flag>>4)&1,(filter_flag>>3)&1,(filter_flag>>2)&1,(filter_flag>>1)&1);
}

static int openwifi_ampdu_action(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_ampdu_params *params)
{
  struct ieee80211_sta *sta = params->sta;
  enum ieee80211_ampdu_mlme_action action = params->action;
  // struct openwifi_priv *priv = hw->priv;
  u16 max_tx_bytes, buf_size;
  u32 ampdu_action_config;

  if (!AGGR_ENABLE) {
    return -EOPNOTSUPP;
  }

  switch (action)
  {
    case IEEE80211_AMPDU_TX_START:
      ieee80211_start_tx_ba_cb_irqsafe(vif, sta->addr, params->tid);
      printk("%s openwifi_ampdu_action: start TX aggregation. tid %d\n", sdr_compatible_str, params->tid);
      break;
    case IEEE80211_AMPDU_TX_STOP_CONT:
    case IEEE80211_AMPDU_TX_STOP_FLUSH:
    case IEEE80211_AMPDU_TX_STOP_FLUSH_CONT:
      ieee80211_stop_tx_ba_cb_irqsafe(vif, sta->addr, params->tid);
      printk("%s openwifi_ampdu_action: stop TX aggregation. tid %d\n", sdr_compatible_str, params->tid);
      break;
    case IEEE80211_AMPDU_TX_OPERATIONAL:
      buf_size = 4;
//      buf_size = (params->buf_size) - 1;
      max_tx_bytes = (1 << (IEEE80211_HT_MAX_AMPDU_FACTOR + sta->ht_cap.ampdu_factor)) - 1;
      ampdu_action_config = ( sta->ht_cap.ampdu_density<<24 | buf_size<<16 | max_tx_bytes );
      tx_intf_api->TX_INTF_REG_AMPDU_ACTION_CONFIG_write(ampdu_action_config);
      printk("%s openwifi_ampdu_action: TX operational. tid %d max_tx_bytes %d ampdu_density %d buf_size %d\n", 
      sdr_compatible_str, params->tid, max_tx_bytes, sta->ht_cap.ampdu_density, buf_size);
      break;
    case IEEE80211_AMPDU_RX_START:
      printk("%s openwifi_ampdu_action: start RX aggregation. tid %d\n", sdr_compatible_str, params->tid);
      break;
    case IEEE80211_AMPDU_RX_STOP:
      printk("%s openwifi_ampdu_action: stop RX aggregation. tid %d\n", sdr_compatible_str, params->tid);
      break;
    default:
      return -EOPNOTSUPP;
  }

  return 0;
}

static const struct ieee80211_ops openwifi_ops = {
  .tx             = openwifi_tx,
  .start         = openwifi_start,
  .stop         = openwifi_stop,
  .add_interface     = openwifi_add_interface,
  .remove_interface  = openwifi_remove_interface,
  .config         = openwifi_config,
  .set_antenna       = openwifi_set_antenna,
  .get_antenna       = openwifi_get_antenna,
  .bss_info_changed  = openwifi_bss_info_changed,
  .conf_tx       = openwifi_conf_tx,
  .prepare_multicast = openwifi_prepare_multicast,
  .configure_filter  = openwifi_configure_filter,
  .rfkill_poll     = openwifi_rfkill_poll,
  .get_tsf       = openwifi_get_tsf,
  .set_tsf       = openwifi_set_tsf,
  .reset_tsf       = openwifi_reset_tsf,
  .set_rts_threshold = openwifi_set_rts_threshold,
  .ampdu_action      = openwifi_ampdu_action,
  .testmode_cmd     = openwifi_testmode_cmd,
};

static const struct of_device_id openwifi_dev_of_ids[] = {
  { .compatible = "sdr,sdr", },
  {}
};
MODULE_DEVICE_TABLE(of, openwifi_dev_of_ids);

static int custom_match_spi_dev(struct device *dev, const void *data)
{
  const char *name = data;

  bool ret = sysfs_streq(name, dev->of_node->name);
  printk("%s custom_match_spi_dev %s %s %d\n", sdr_compatible_str,name, dev->of_node->name, ret);
  return ret;
}

static int custom_match_platform_dev(struct device *dev, const void *data)
{
  struct platform_device *plat_dev = to_platform_device(dev);
  const char *name = data;
  char *name_in_sys_bus_platform_devices = strstr(plat_dev->name, name);
  bool match_flag = (name_in_sys_bus_platform_devices != NULL);

  if (match_flag) {
    printk("%s custom_match_platform_dev %s\n", sdr_compatible_str,plat_dev->name);
  }
  return(match_flag);
}

static int openwifi_dev_probe(struct platform_device *pdev)
{
  struct ieee80211_hw *dev;
  struct openwifi_priv *priv;
  struct device_node *dt_node;
  int err=1, rand_val;
  const char *fpga_model;
  u32 reg, i;//, reg1;

  struct device_node *np = pdev->dev.of_node;

  struct device *tmp_dev;
  struct platform_device *tmp_pdev;
  struct iio_dev *tmp_indio_dev;
  // struct gpio_leds_priv *tmp_led_priv;

  printk("\n");

  if (np) {
    const struct of_device_id *match;

    match = of_match_node(openwifi_dev_of_ids, np);
    if (match) {
      printk("%s openwifi_dev_probe: match!\n", sdr_compatible_str);
      err = 0;
    }
  }

  if (err)
    return err;

  dev = ieee80211_alloc_hw(sizeof(*priv), &openwifi_ops);
  if (!dev) {
    printk(KERN_ERR "%s openwifi_dev_probe: ieee80211 alloc failed\n",sdr_compatible_str);
    err = -ENOMEM;
    goto err_free_dev;
  }

  priv = dev->priv;
  priv->pdev = pdev;

  err = of_property_read_string(of_find_node_by_path("/"), "model", &fpga_model);
  if(err < 0) {
    priv->hardware_type = UNKNOWN_HARDWARE;
    priv->fpga_type = SMALL_FPGA;
    printk("%s openwifi_dev_probe: WARNING unknown openwifi FPGA model %d\n",sdr_compatible_str, err);
    printk("%s openwifi_dev_probe: Try to detect TI lmk04828. If it exist, treate the board as RFSoC4x2\n",sdr_compatible_str);
    dt_node = of_find_node_by_name(NULL, "lmk");
    if (dt_node != NULL) {
      printk("%s openwifi_dev_probe: found device tree node name %s\n",sdr_compatible_str, dt_node->name);
      priv->hardware_type = RFSOC4X2;
      priv->fpga_type = LARGE_FPGA;
    } else {
      printk("%s openwifi_dev_probe: WARNING device tree lmk node is not detected! %d\n",sdr_compatible_str, err);
    }
  } else {
    if(strstr(fpga_model, "ZCU102") != NULL) {
      priv->hardware_type = ZYNQMP_AD9361;
    } else {
      priv->hardware_type = ZYNQ_AD9361;
    }

    // LARGE FPGAs (i.e. ZCU102, Z7035, ZC706)
    if(strstr(fpga_model, "ZCU102") != NULL || strstr(fpga_model, "Z7035") != NULL || strstr(fpga_model, "ZC706") != NULL) {
      priv->fpga_type = LARGE_FPGA;
    // SMALL FPGA: (i.e. ZED, ZC702, Z7020)
    }//  else if(strstr(fpga_model, "ZED") != NULL || strstr(fpga_model, "ZC702") != NULL || strstr(fpga_model, "Z7020") != NULL) {
    else { // ALL others are SAMLL_FPGA
      priv->fpga_type = SMALL_FPGA;
    }
  }

  priv->actual_rx_lo = 1000; //Some value aligned with rf_init/rf_init_11n.sh that is not WiFi channel to force ad9361_rf_set_channel execution triggered by Linux
  priv->actual_tx_lo = 1000; //Some value aligned with rf_init/rf_init_11n.sh that is not WiFi channel to force ad9361_rf_set_channel execution triggered by Linux
  priv->band = freq_MHz_to_band(priv->actual_rx_lo);
  priv->use_short_slot = false; //this can be changed by openwifi_bss_info_changed: BSS_CHANGED_ERP_SLOT
  priv->ampdu_reference = 0;
  priv->last_tx_quad_cal_lo = 1000;

  if (priv->hardware_type != RFSOC4X2) {
    // //-------------find ad9361-phy driver for lo/channel control---------------
    tmp_dev = bus_find_device( &spi_bus_type, NULL, "ad9361-phy", custom_match_spi_dev );
    if (tmp_dev == NULL) {
      printk(KERN_ERR "%s find_dev ad9361-phy failed\n",sdr_compatible_str);
      err = -ENODEV;
      goto err_free_dev;
    }
    printk("%s bus_find_device ad9361-phy: %s. driver_data pointer %p\n", sdr_compatible_str, ((struct spi_device*)tmp_dev)->modalias, (void*)(((struct spi_device*)tmp_dev)->dev.driver_data));
    if (((struct spi_device*)tmp_dev)->dev.driver_data == NULL) {
      printk(KERN_ERR "%s find_dev ad9361-phy failed. dev.driver_data == NULL\n",sdr_compatible_str);
      err = -ENODEV;
      goto err_free_dev;
    }
    
    priv->ad9361_phy = ad9361_spi_to_phy((struct spi_device*)tmp_dev);
    if (!(priv->ad9361_phy)) {
      printk(KERN_ERR "%s ad9361_spi_to_phy failed\n",sdr_compatible_str);
      err = -ENODEV;
      goto err_free_dev;
    }
    printk("%s ad9361_spi_to_phy ad9361-phy: %s\n", sdr_compatible_str, priv->ad9361_phy->spi->modalias);

    // //-------------find driver: axi_ad9361 hdl ref design module, dac channel---------------
    tmp_dev = bus_find_device( &platform_bus_type, NULL, "cf-ad9361-dds-core-lpc", custom_match_platform_dev );
    if (!tmp_dev) {
      printk(KERN_ERR "%s bus_find_device platform_bus_type cf-ad9361-dds-core-lpc failed\n",sdr_compatible_str);
      err = -ENODEV;
      goto err_free_dev;
    }

    tmp_pdev = to_platform_device(tmp_dev);
    if (!tmp_pdev) {
      printk(KERN_ERR "%s to_platform_device failed\n",sdr_compatible_str);
      err = -ENODEV;
      goto err_free_dev;
    }

    tmp_indio_dev = platform_get_drvdata(tmp_pdev);
    if (!tmp_indio_dev) {
      printk(KERN_ERR "%s platform_get_drvdata failed\n",sdr_compatible_str);
      err = -ENODEV;
      goto err_free_dev;
    }

    priv->dds_st = iio_priv(tmp_indio_dev);
    if (!(priv->dds_st)) {
      printk(KERN_ERR "%s iio_priv failed\n",sdr_compatible_str);
      err = -ENODEV;
      goto err_free_dev;
    }
    printk("%s openwifi_dev_probe: cf-ad9361-dds-core-lpc dds_st->version %08x chip_info->name %s\n",sdr_compatible_str,priv->dds_st->version,priv->dds_st->chip_info->name);
    cf_axi_dds_datasel(priv->dds_st, -1, DATA_SEL_DMA);
    printk("%s openwifi_dev_probe: cf_axi_dds_datasel DATA_SEL_DMA\n",sdr_compatible_str);

    // //-------------find driver: axi_ad9361 hdl ref design module, adc channel---------------
    // turn off radio by muting tx
    // ad9361_tx_mute(priv->ad9361_phy, 1);
    // reg = ad9361_get_tx_atten(priv->ad9361_phy, 2);
    // reg1 = ad9361_get_tx_atten(priv->ad9361_phy, 1);
    // if (reg == AD9361_RADIO_OFF_TX_ATT && reg1 == AD9361_RADIO_OFF_TX_ATT ) {
    //   priv->rfkill_off = 0;// 0 off, 1 on
    //   printk("%s openwifi_dev_probe: rfkill radio off\n",sdr_compatible_str);
    // }
    // else
    //   printk("%s openwifi_dev_probe: WARNING rfkill radio off failed. tx att read %d %d require %d\n",sdr_compatible_str, reg, reg1, AD9361_RADIO_OFF_TX_ATT);
  } else { //construct a fake ad9361_phy as a temporary solution
    priv->ad9361_phy = &ad9361_phy_fake;
    priv->ad9361_phy->state = &ad9361_phy_state_fake;
  }

  // //-----------------------------parse the test_mode input--------------------------------
  if (test_mode&1)
    AGGR_ENABLE = true;
  
  // if (test_mode&2)
  //   TX_OFFSET_TUNING_ENABLE = false;

  priv->rssi_correction = rssi_correction_lookup_table(5220);//5220MHz. this will be set in real-time by _rf_set_channel()
  priv->last_auto_fpga_lbt_th = rssi_dbm_to_rssi_half_db(-78, priv->rssi_correction);//-78dBm. a magic value. just to avoid uninitialized

  //priv->rf_bw = 20000000; // Signal quality issue! NOT use for now. 20MHz or 40MHz. 40MHz need ddc/duc. 20MHz works in bypass mode
  priv->rf_bw = 40000000; // 20MHz or 40MHz. 40MHz need ddc/duc. 20MHz works in bypass mode

  priv->xpu_cfg = XPU_NORMAL;

  priv->openofdm_tx_cfg = OPENOFDM_TX_NORMAL;
  priv->openofdm_rx_cfg = OPENOFDM_RX_NORMAL;

  printk("%s openwifi_dev_probe: priv->rf_bw == %dHz. bool for 20000000 %d, 40000000 %d\n",sdr_compatible_str, priv->rf_bw, (priv->rf_bw==20000000) , (priv->rf_bw==40000000) );
  if (priv->rf_bw == 20000000) { //DO NOT USE. Not used for long time.
    priv->rx_intf_cfg = RX_INTF_BYPASS;
    priv->tx_intf_cfg = TX_INTF_BYPASS;
    //priv->rx_freq_offset_to_lo_MHz = 0;
    //priv->tx_freq_offset_to_lo_MHz = 0;
  } else if (priv->rf_bw == 40000000) {
    //priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_P_10MHZ; //work
    //priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1; //work

    // // test ddc at central, duc at central+10M. It works. And also change rx BW from 40MHz to 20MHz in rf_init.sh. Rx sampling rate is still 40Msps
    priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_0MHZ_ANT0;
    if (TX_OFFSET_TUNING_ENABLE)
      priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0; // Let's use rx0 tx0 as default mode, because it works for both 9361 and 9364
    else
      priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_0MHZ_ANT0;
    // // try another antenna option
    //priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_0MHZ_ANT1;
    //priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0;
    
    #if 0
    if (priv->rx_intf_cfg == DDC_BW_20MHZ_AT_N_10MHZ) {
      priv->rx_freq_offset_to_lo_MHz = -10;
    } else if (priv->rx_intf_cfg == DDC_BW_20MHZ_AT_P_10MHZ) {
      priv->rx_freq_offset_to_lo_MHz = 10;
    } else if (priv->rx_intf_cfg == DDC_BW_20MHZ_AT_0MHZ) {
      priv->rx_freq_offset_to_lo_MHz = 0;
    } else {
      printk("%s openwifi_dev_probe: Warning! priv->rx_intf_cfg == %d\n",sdr_compatible_str,priv->rx_intf_cfg);
    }
    #endif
  } else {
    printk("%s openwifi_dev_probe: Warning! priv->rf_bw == %dHz (should be 20000000 or 40000000)\n",sdr_compatible_str, priv->rf_bw);
    err = -EBADRQC;
    goto err_free_dev;
  }

  printk("%s openwifi_dev_probe: test_mode %x AGGR_ENABLE %d TX_OFFSET_TUNING_ENABLE %d init_tx_att %d\n", sdr_compatible_str, test_mode, AGGR_ENABLE, TX_OFFSET_TUNING_ENABLE, init_tx_att);

  priv->runtime_tx_ant_cfg = ((priv->tx_intf_cfg==TX_INTF_BW_20MHZ_AT_0MHZ_ANT0 || priv->tx_intf_cfg==TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0)?1:(priv->tx_intf_cfg==TX_INTF_BW_20MHZ_AT_0MHZ_ANT_BOTH?3:2));
  priv->runtime_rx_ant_cfg = (priv->rx_intf_cfg==RX_INTF_BW_20MHZ_AT_0MHZ_ANT0?1:2);

  priv->ctrl_out.en_mask=AD9361_CTRL_OUT_EN_MASK;
  priv->ctrl_out.index  =(priv->rx_intf_cfg==RX_INTF_BW_20MHZ_AT_0MHZ_ANT0?AD9361_CTRL_OUT_INDEX_ANT0:AD9361_CTRL_OUT_INDEX_ANT1);

  memset(priv->drv_rx_reg_val,0,sizeof(priv->drv_rx_reg_val));
  memset(priv->drv_tx_reg_val,0,sizeof(priv->drv_tx_reg_val));
  memset(priv->drv_xpu_reg_val,0,sizeof(priv->drv_xpu_reg_val));
  memset(priv->rf_reg_val,0,sizeof(priv->rf_reg_val));

  priv->rf_reg_val[RF_TX_REG_IDX_ATT] = init_tx_att;

  //let's by default turn radio on when probing
  err = openwifi_set_antenna(dev, priv->runtime_tx_ant_cfg, priv->runtime_rx_ant_cfg);
  if (err) {
    printk("%s openwifi_dev_probe: WARNING openwifi_set_antenna FAIL %d\n",sdr_compatible_str, err);
    err = -EIO;
    goto err_free_dev;
  }
  reg = ad9361_spi_read(priv->ad9361_phy->spi, REG_CTRL_OUTPUT_POINTER);
  printk("%s openwifi_dev_probe: ad9361_spi_read REG_CTRL_OUTPUT_POINTER 0x%02x\n",sdr_compatible_str, reg);
  reg = ad9361_spi_read(priv->ad9361_phy->spi, REG_CTRL_OUTPUT_ENABLE);
  printk("%s openwifi_dev_probe: ad9361_spi_read REG_CTRL_OUTPUT_ENABLE 0x%02x\n",sdr_compatible_str, reg);

  reg = ad9361_get_tx_atten(priv->ad9361_phy, ((priv->runtime_tx_ant_cfg==1 || priv->runtime_tx_ant_cfg==3)?1:2));
  if (reg == (AD9361_RADIO_ON_TX_ATT+priv->rf_reg_val[RF_TX_REG_IDX_ATT])) {
    priv->rfkill_off = 1;// 0 off, 1 on
    printk("%s openwifi_dev_probe: rfkill radio on\n",sdr_compatible_str);
  } else
    printk("%s openwifi_dev_probe: WARNING rfkill radio on failed. tx att read %d require %d\n",sdr_compatible_str, reg, AD9361_RADIO_ON_TX_ATT+priv->rf_reg_val[RF_TX_REG_IDX_ATT]);

  priv->drv_xpu_reg_val[DRV_XPU_REG_IDX_GIT_REV] = GIT_REV;

  // //set ad9361 in certain mode
  #if 0
  err = ad9361_set_trx_clock_chain_freq(priv->ad9361_phy,priv->rf_bw);
  printk("%s openwifi_dev_probe: ad9361_set_trx_clock_chain_freq %dHz err %d\n",sdr_compatible_str, priv->rf_bw,err);
  err = ad9361_update_rf_bandwidth(priv->ad9361_phy,priv->rf_bw,priv->rf_bw);
  printk("%s openwifi_dev_probe: ad9361_update_rf_bandwidth %dHz err %d\n",sdr_compatible_str, priv->rf_bw,err);

  rx_intf_api->hw_init(priv->rx_intf_cfg,8,8);
  tx_intf_api->hw_init(priv->tx_intf_cfg,8,8,priv->fpga_type);
  openofdm_tx_api->hw_init(priv->openofdm_tx_cfg);
  openofdm_rx_api->hw_init(priv->openofdm_rx_cfg);
  printk("%s openwifi_dev_probe: rx_intf_cfg %d openofdm_rx_cfg %d tx_intf_cfg %d openofdm_tx_cfg %d\n",sdr_compatible_str, priv->rx_intf_cfg, priv->openofdm_rx_cfg, priv->tx_intf_cfg, priv->openofdm_tx_cfg);
  printk("%s openwifi_dev_probe: rx_freq_offset_to_lo_MHz %d tx_freq_offset_to_lo_MHz %d\n",sdr_compatible_str, priv->rx_freq_offset_to_lo_MHz, priv->tx_freq_offset_to_lo_MHz);
  #endif

  dev->max_rates = 1; //maximum number of alternate rate retry stages the hw can handle.

  SET_IEEE80211_DEV(dev, &pdev->dev);
  platform_set_drvdata(pdev, dev);

  BUILD_BUG_ON(sizeof(priv->rates_2GHz) != sizeof(openwifi_2GHz_rates));
  BUILD_BUG_ON(sizeof(priv->rates_5GHz) != sizeof(openwifi_5GHz_rates));
  BUILD_BUG_ON(sizeof(priv->channels_2GHz) != sizeof(openwifi_2GHz_channels));
  BUILD_BUG_ON(sizeof(priv->channels_5GHz) != sizeof(openwifi_5GHz_channels));

  memcpy(priv->rates_2GHz, openwifi_2GHz_rates, sizeof(openwifi_2GHz_rates));
  memcpy(priv->rates_5GHz, openwifi_5GHz_rates, sizeof(openwifi_5GHz_rates));
  memcpy(priv->channels_2GHz, openwifi_2GHz_channels, sizeof(openwifi_2GHz_channels));
  memcpy(priv->channels_5GHz, openwifi_5GHz_channels, sizeof(openwifi_5GHz_channels));

  priv->band_2GHz.band = NL80211_BAND_2GHZ;
  priv->band_2GHz.channels = priv->channels_2GHz;
  priv->band_2GHz.n_channels = ARRAY_SIZE(priv->channels_2GHz);
  priv->band_2GHz.bitrates = priv->rates_2GHz;
  priv->band_2GHz.n_bitrates = ARRAY_SIZE(priv->rates_2GHz);
  priv->band_2GHz.ht_cap.ht_supported = true;

  if (test_mode&2)
    priv->band_2GHz.ht_cap.cap = IEEE80211_HT_CAP_SGI_20; //SGI -- short GI seems bring unnecessary stability issue
  
  if (AGGR_ENABLE) {
    priv->band_2GHz.ht_cap.ampdu_factor = IEEE80211_HT_MAX_AMPDU_8K;
    priv->band_2GHz.ht_cap.ampdu_density = IEEE80211_HT_MPDU_DENSITY_2;
  }
  memset(&priv->band_2GHz.ht_cap.mcs, 0, sizeof(priv->band_2GHz.ht_cap.mcs));
  priv->band_2GHz.ht_cap.mcs.rx_mask[0] = 0xff;
  priv->band_2GHz.ht_cap.mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;
  dev->wiphy->bands[NL80211_BAND_2GHZ] = &(priv->band_2GHz);

  priv->band_5GHz.band = NL80211_BAND_5GHZ;
  priv->band_5GHz.channels = priv->channels_5GHz;
  priv->band_5GHz.n_channels = ARRAY_SIZE(priv->channels_5GHz);
  priv->band_5GHz.bitrates = priv->rates_5GHz;
  priv->band_5GHz.n_bitrates = ARRAY_SIZE(priv->rates_5GHz);
  priv->band_5GHz.ht_cap.ht_supported = true;

  if (test_mode&2)
    priv->band_5GHz.ht_cap.cap = IEEE80211_HT_CAP_SGI_20; //SGI -- short GI seems bring unnecessary stability issue
  
  if (AGGR_ENABLE) {
    priv->band_5GHz.ht_cap.ampdu_factor = IEEE80211_HT_MAX_AMPDU_8K;
    priv->band_5GHz.ht_cap.ampdu_density = IEEE80211_HT_MPDU_DENSITY_2;
  }
  memset(&priv->band_5GHz.ht_cap.mcs, 0, sizeof(priv->band_5GHz.ht_cap.mcs));
  priv->band_5GHz.ht_cap.mcs.rx_mask[0] = 0xff;
  priv->band_5GHz.ht_cap.mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;
  dev->wiphy->bands[NL80211_BAND_5GHZ] = &(priv->band_5GHz);

  printk("%s openwifi_dev_probe: band_2GHz.n_channels %d n_bitrates %d band_5GHz.n_channels %d n_bitrates %d\n",sdr_compatible_str,
  priv->band_2GHz.n_channels,priv->band_2GHz.n_bitrates,priv->band_5GHz.n_channels,priv->band_5GHz.n_bitrates);

  // ieee80211_hw_set(dev, HOST_BROADCAST_PS_BUFFERING); // remove this because we don't want: mac80211.h: host buffers frame for PS and we fetch them via ieee80211_get_buffered_bc()
  ieee80211_hw_set(dev, RX_INCLUDES_FCS);
  ieee80211_hw_set(dev, BEACON_TX_STATUS);//mac80211.h: The device/driver provides TX status for sent beacons.

  ieee80211_hw_set(dev, REPORTS_TX_ACK_STATUS);//mac80211.h: Hardware can provide ack status reports of Tx frames to the stack

  // * @IEEE80211_HW_AP_LINK_PS: When operating in AP mode the device
  // *  autonomously manages the PS status of connected stations. When
  // *  this flag is set mac80211 will not trigger PS mode for connected
  // *  stations based on the PM bit of incoming frames.
  // *  Use ieee80211_start_ps()/ieee8021_end_ps() to manually configure
  // *  the PS mode of connected stations.
  ieee80211_hw_set(dev, AP_LINK_PS);

  if (AGGR_ENABLE) {
    ieee80211_hw_set(dev, AMPDU_AGGREGATION);
  }

  dev->extra_tx_headroom = LEN_MPDU_DELIM;

  dev->vif_data_size = sizeof(struct openwifi_vif);
  dev->wiphy->interface_modes = 
      BIT(NL80211_IFTYPE_MONITOR)|
      BIT(NL80211_IFTYPE_P2P_GO) |
      BIT(NL80211_IFTYPE_P2P_CLIENT) |
      BIT(NL80211_IFTYPE_AP) |
      BIT(NL80211_IFTYPE_STATION) |
      BIT(NL80211_IFTYPE_ADHOC) |
      BIT(NL80211_IFTYPE_MESH_POINT) |
      BIT(NL80211_IFTYPE_OCB);
  dev->wiphy->iface_combinations = &openwifi_if_comb;
  dev->wiphy->n_iface_combinations = 1;

  dev->wiphy->available_antennas_tx = NUM_TX_ANT_MASK;
  dev->wiphy->available_antennas_rx = NUM_RX_ANT_MASK;

  dev->wiphy->regulatory_flags = (REGULATORY_STRICT_REG|REGULATORY_CUSTOM_REG); // use our own config within strict regulation
  //dev->wiphy->regulatory_flags = REGULATORY_CUSTOM_REG; // use our own config
  wiphy_apply_custom_regulatory(dev->wiphy, &sdr_regd);

  /* we declare to MAC80211 all the queues except for beacon queue
   * that will be eventually handled by DRV.
   * TX rings are arranged in such a way that lower is the IDX,
   * higher is the priority, in order to achieve direct mapping
   * with mac80211, however the beacon queue is an exception and it
   * is mapped on the highst tx ring IDX.
   */
  dev->queues = MAX_NUM_HW_QUEUE;

  ieee80211_hw_set(dev, SIGNAL_DBM);

  wiphy_ext_feature_set(dev->wiphy, NL80211_EXT_FEATURE_CQM_RSSI_LIST);

  if (priv->hardware_type == RFSOC4X2) {
    priv->rf = &rfsoc4x2_rf_ops;
  } else {
    priv->rf = &ad9361_rf_ops;
  }

  memset(priv->dest_mac_addr_queue_map,0,sizeof(priv->dest_mac_addr_queue_map));
  priv->slice_idx = 0xFFFFFFFF;

  sg_init_table(&(priv->tx_sg), 1);

  get_random_bytes(&rand_val, sizeof(rand_val));
    rand_val%=250;
  priv->mac_addr[0]=0x66;  priv->mac_addr[1]=0x55;  priv->mac_addr[2]=0x44;  priv->mac_addr[3]=0x33;  priv->mac_addr[4]=0x22;
  priv->mac_addr[5]=rand_val+1;
  //priv->mac_addr[5]=0x11;
  if (!is_valid_ether_addr(priv->mac_addr)) {
    printk(KERN_WARNING "%s openwifi_dev_probe: WARNING Invalid hwaddr! Using randomly generated MAC addr\n",sdr_compatible_str);
    eth_random_addr(priv->mac_addr);
  }
  printk("%s openwifi_dev_probe: mac_addr %02x:%02x:%02x:%02x:%02x:%02x\n",sdr_compatible_str,priv->mac_addr[0],priv->mac_addr[1],priv->mac_addr[2],priv->mac_addr[3],priv->mac_addr[4],priv->mac_addr[5]);
  SET_IEEE80211_PERM_ADDR(dev, priv->mac_addr);

  spin_lock_init(&priv->lock);

  err = ieee80211_register_hw(dev);
  if (err) {
    pr_err(KERN_ERR "%s openwifi_dev_probe: WARNING Cannot register device\n",sdr_compatible_str);
    err = -EIO;
    goto err_free_dev;
  } else {
    printk("%s openwifi_dev_probe: ieee80211_register_hw %d\n",sdr_compatible_str, err);
  }

  // create sysfs for arbitrary iq setting
  sysfs_bin_attr_init(&priv->bin_iq);
  priv->bin_iq.attr.name = "tx_intf_iq_data";
  priv->bin_iq.attr.mode = S_IWUSR | S_IRUGO;
  priv->bin_iq.write = openwifi_tx_intf_bin_iq_write;
  priv->bin_iq.read = openwifi_tx_intf_bin_iq_read;
  priv->bin_iq.size = 4096;
  err = sysfs_create_bin_file(&pdev->dev.kobj, &priv->bin_iq);
  printk("%s openwifi_dev_probe: sysfs_create_bin_file %d\n",sdr_compatible_str, err);
  if (err < 0)
    goto err_free_dev;

  priv->tx_intf_arbitrary_iq_num = 0;
  // priv->tx_intf_arbitrary_iq[0] = 1;
  // priv->tx_intf_arbitrary_iq[1] = 2;

  err = sysfs_create_group(&pdev->dev.kobj, &tx_intf_attribute_group);
  printk("%s openwifi_dev_probe: sysfs_create_group tx_intf_attribute_group %d\n",sdr_compatible_str, err);
  if (err < 0)
    goto err_free_dev;
  priv->tx_intf_iq_ctl = 0;

  // create sysfs for stat
  err = sysfs_create_group(&pdev->dev.kobj, &stat_attribute_group);
  printk("%s openwifi_dev_probe: sysfs_create_group stat_attribute_group %d\n",sdr_compatible_str, err);
  if (err < 0)
    goto err_free_dev;

  priv->stat.stat_enable = 0; // by default disable
  
  for (i=0; i<MAX_NUM_SW_QUEUE; i++) {
    priv->stat.tx_prio_num[i] = 0;
    priv->stat.tx_prio_interrupt_num[i] = 0;
    priv->stat.tx_prio_stop0_fake_num[i] = 0;
    priv->stat.tx_prio_stop0_real_num[i] = 0;
    priv->stat.tx_prio_stop1_num[i] = 0;
    priv->stat.tx_prio_wakeup_num[i] = 0;
  }
  for (i=0; i<MAX_NUM_HW_QUEUE; i++) {
    priv->stat.tx_queue_num[i] = 0;
    priv->stat.tx_queue_interrupt_num[i] = 0;
    priv->stat.tx_queue_stop0_fake_num[i] = 0;
    priv->stat.tx_queue_stop0_real_num[i] = 0;
    priv->stat.tx_queue_stop1_num[i] = 0;
    priv->stat.tx_queue_wakeup_num[i] = 0;
  }
    
  priv->stat.tx_data_pkt_need_ack_num_total = 0;
  priv->stat.tx_data_pkt_need_ack_num_total_fail = 0;
  for (i=0; i<6; i++) {
    priv->stat.tx_data_pkt_need_ack_num_retx[i] = 0;
    priv->stat.tx_data_pkt_need_ack_num_retx_fail[i] = 0;
  }
  priv->stat.tx_data_pkt_mcs_realtime = 0;
  priv->stat.tx_data_pkt_fail_mcs_realtime = 0;

  priv->stat.tx_mgmt_pkt_need_ack_num_total = 0;
  priv->stat.tx_mgmt_pkt_need_ack_num_total_fail = 0;
  for (i=0; i<3; i++) {
    priv->stat.tx_mgmt_pkt_need_ack_num_retx[i] = 0;
    priv->stat.tx_mgmt_pkt_need_ack_num_retx_fail[i] = 0;
  }
  priv->stat.tx_mgmt_pkt_mcs_realtime = 0;
  priv->stat.tx_mgmt_pkt_fail_mcs_realtime = 0;

  priv->stat.rx_monitor_all = 0;
  priv->stat.rx_target_sender_mac_addr = 0;
  priv->stat.rx_data_ok_agc_gain_value_realtime = 0;
  priv->stat.rx_data_fail_agc_gain_value_realtime = 0;
  priv->stat.rx_mgmt_ok_agc_gain_value_realtime = 0;
  priv->stat.rx_mgmt_fail_agc_gain_value_realtime = 0;
  priv->stat.rx_ack_ok_agc_gain_value_realtime = 0;

  priv->stat.rx_monitor_all = 0;
  priv->stat.rx_data_pkt_num_total = 0;
  priv->stat.rx_data_pkt_num_fail = 0;
  priv->stat.rx_mgmt_pkt_num_total = 0;
  priv->stat.rx_mgmt_pkt_num_fail = 0;
  priv->stat.rx_ack_pkt_num_total = 0;
  priv->stat.rx_ack_pkt_num_fail = 0;

  priv->stat.rx_data_pkt_mcs_realtime = 0;
  priv->stat.rx_data_pkt_fail_mcs_realtime = 0;
  priv->stat.rx_mgmt_pkt_mcs_realtime = 0;
  priv->stat.rx_mgmt_pkt_fail_mcs_realtime = 0;
  priv->stat.rx_ack_pkt_mcs_realtime = 0;

  priv->stat.restrict_freq_mhz = 0;

  priv->stat.csma_cfg0 = 0;
  priv->stat.cw_max_min_cfg = 0;

  priv->stat.dbg_ch0 = 0;
  priv->stat.dbg_ch1 = 0;
  priv->stat.dbg_ch2 = 0;

  // // //--------------------hook leds (not complete yet)--------------------------------
  // tmp_dev = bus_find_device( &platform_bus_type, NULL, "leds", custom_match_platform_dev ); //leds is the name in devicetree, not "compatible" field
  // if (!tmp_dev) {
  //   printk(KERN_ERR "%s bus_find_device platform_bus_type leds-gpio failed\n",sdr_compatible_str);
  //   err = -ENOMEM;
  //   goto err_free_dev;
  // }

  // tmp_pdev = to_platform_device(tmp_dev);
  // if (!tmp_pdev) {
  //   printk(KERN_ERR "%s to_platform_device failed for leds-gpio\n",sdr_compatible_str);
  //   err = -ENOMEM;
  //   goto err_free_dev;
  // }

  // tmp_led_priv = platform_get_drvdata(tmp_pdev);
  // if (!tmp_led_priv) {
  //   printk(KERN_ERR "%s platform_get_drvdata failed for leds-gpio\n",sdr_compatible_str);
  //   err = -ENOMEM;
  //   goto err_free_dev;
  // }
  // printk("%s openwifi_dev_probe: leds-gpio detect %d leds!\n",sdr_compatible_str, tmp_led_priv->num_leds);
  // if (tmp_led_priv->num_leds!=4){
  //   printk(KERN_ERR "%s WARNING we expect 4 leds, but actual %d leds\n",sdr_compatible_str,tmp_led_priv->num_leds);
  //   err = -ENOMEM;
  //   goto err_free_dev;
  // }
  // gpiod_set_value(tmp_led_priv->leds[0].gpiod, 1);//light it
  // gpiod_set_value(tmp_led_priv->leds[3].gpiod, 0);//black it
  // priv->num_led = tmp_led_priv->num_leds;
  // priv->led[0] = &(tmp_led_priv->leds[0].cdev);
  // priv->led[1] = &(tmp_led_priv->leds[1].cdev);
  // priv->led[2] = &(tmp_led_priv->leds[2].cdev);
  // priv->led[3] = &(tmp_led_priv->leds[3].cdev);

  // snprintf(priv->led_name[0], OPENWIFI_LED_MAX_NAME_LEN, "openwifi-%s::radio", wiphy_name(dev->wiphy));
  // snprintf(priv->led_name[1], OPENWIFI_LED_MAX_NAME_LEN, "openwifi-%s::assoc", wiphy_name(dev->wiphy));
  // snprintf(priv->led_name[2], OPENWIFI_LED_MAX_NAME_LEN, "openwifi-%s::tx", wiphy_name(dev->wiphy));
  // snprintf(priv->led_name[3], OPENWIFI_LED_MAX_NAME_LEN, "openwifi-%s::rx", wiphy_name(dev->wiphy));
  
  wiphy_info(dev->wiphy, "hwaddr %pm, FPGA %s\n",
       priv->mac_addr, priv->rf->name);

  openwifi_rfkill_init(dev);
  return 0;

 err_free_dev:
  ieee80211_free_hw(dev);

  return err;
}

static int openwifi_dev_remove(struct platform_device *pdev)
{
  struct ieee80211_hw *dev = platform_get_drvdata(pdev);
  struct openwifi_priv *priv = dev->priv;

  if (!dev) {
    pr_info("%s openwifi_dev_remove: dev %p\n", sdr_compatible_str, (void*)dev);
    return(-1);
  }

  sysfs_remove_bin_file(&pdev->dev.kobj, &priv->bin_iq);
  sysfs_remove_group(&pdev->dev.kobj, &tx_intf_attribute_group);
  sysfs_remove_group(&pdev->dev.kobj, &stat_attribute_group);

  openwifi_rfkill_exit(dev);
  ieee80211_unregister_hw(dev);
  ieee80211_free_hw(dev);
  return(0);
}

static struct platform_driver openwifi_dev_driver = {
  .driver = {
    .name = "sdr,sdr",
    .owner = THIS_MODULE,
    .of_match_table = openwifi_dev_of_ids,
  },
  .probe = openwifi_dev_probe,
  .remove = openwifi_dev_remove,
};

module_platform_driver(openwifi_dev_driver);
