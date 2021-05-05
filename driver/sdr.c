// Author: Xianjun Jiao, Michael Mehari, Wei Liu
// SPDX-FileCopyrightText: 2019 UGent
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

#define IIO_AD9361_USE_PRIVATE_H_
#include <../../drivers/iio/adc/ad9361_regs.h>
#include <../../drivers/iio/adc/ad9361.h>
#include <../../drivers/iio/adc/ad9361_private.h>

#include <../../drivers/iio/frequency/cf_axi_dds.h>
extern int ad9361_get_tx_atten(struct ad9361_rf_phy *phy, u32 tx_num);
extern int ad9361_set_tx_atten(struct ad9361_rf_phy *phy, u32 atten_mdb,
			       bool tx1, bool tx2, bool immed);
extern int ad9361_ctrl_outs_setup(struct ad9361_rf_phy *phy,
				  struct ctrl_outs_control *ctrl);

#include "../user_space/sdrctl_src/nl80211_testmode_def.h"
#include "hw_def.h"
#include "sdr.h"
#include "git_rev.h"

// driver API of component driver
extern struct tx_intf_driver_api *tx_intf_api;
extern struct rx_intf_driver_api *rx_intf_api;
extern struct openofdm_tx_driver_api *openofdm_tx_api;
extern struct openofdm_rx_driver_api *openofdm_rx_api;
extern struct xpu_driver_api *xpu_api;

static int test_mode = 0; // 0 normal; 1 rx test

MODULE_AUTHOR("Xianjun Jiao");
MODULE_DESCRIPTION("SDR driver");
MODULE_LICENSE("GPL v2");

module_param(test_mode, int, 0);
MODULE_PARM_DESC(myint, "test_mode. 0 normal; 1 rx test");

// ---------------rfkill---------------------------------------
static bool openwifi_is_radio_enabled(struct openwifi_priv *priv)
{
	int reg;

	if (priv->tx_intf_cfg == TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1)
		reg = ad9361_get_tx_atten(priv->ad9361_phy, 2);
	else
		reg = ad9361_get_tx_atten(priv->ad9361_phy, 1);

	if (reg == AD9361_RADIO_ON_TX_ATT)
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

//static void ad9361_rf_init(void);
//static void ad9361_rf_stop(void);
//static void ad9361_rf_calc_rssi(void);
static void ad9361_rf_set_channel(struct ieee80211_hw *dev,
				  struct ieee80211_conf *conf)
{
	struct openwifi_priv *priv = dev->priv;
	u32 actual_rx_lo = conf->chandef.chan->center_freq - priv->rx_freq_offset_to_lo_MHz + priv->drv_rx_reg_val[DRV_RX_REG_IDX_EXTRA_FO];
	u32 actual_tx_lo;
	bool change_flag = (actual_rx_lo != priv->actual_rx_lo);

	if (change_flag) {
		priv->actual_rx_lo = actual_rx_lo;

		actual_tx_lo = conf->chandef.chan->center_freq - priv->tx_freq_offset_to_lo_MHz;

		clk_set_rate(priv->ad9361_phy->clks[RX_RFPLL], ( ((u64)1000000ull)*((u64)actual_rx_lo )>>1) );
		clk_set_rate(priv->ad9361_phy->clks[TX_RFPLL], ( ((u64)1000000ull)*((u64)actual_tx_lo )>>1) );

		if (actual_rx_lo<2412) {
			priv->rssi_correction = 153;
		} else if (actual_rx_lo<=2484) {
			priv->rssi_correction = 153;
		} else if (actual_rx_lo<5160) {
			priv->rssi_correction = 153;
		} else if (actual_rx_lo<=5240) {
			priv->rssi_correction = 145;
		} else if (actual_rx_lo<=5320) {
			priv->rssi_correction = 148;
		} else {
			priv->rssi_correction = 148;
		}

		// xpu_api->XPU_REG_LBT_TH_write((priv->rssi_correction-62)<<1); // -62dBm
		xpu_api->XPU_REG_LBT_TH_write((priv->rssi_correction-62-16)<<1); // wei's magic value is 135, here is 134 @ ch 44

		if (actual_rx_lo < 2500) {
			//priv->slot_time = 20; //20 is default slot time in ERP(OFDM)/11g 2.4G; short one is 9.
			//xpu_api->XPU_REG_BAND_CHANNEL_write(BAND_2_4GHZ<<16);
			if (priv->band != BAND_2_4GHZ) {
				priv->band = BAND_2_4GHZ;
				xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16) );
			}
			// //xpu_api->XPU_REG_RECV_ACK_COUNT_TOP_write( (((45+2)*10)<<16) | 10 ); // high 16 bits to cover sig valid of ACK packet, low 16 bits is adjustment of fcs valid waiting time.  let's add 2us for those device that is really "slow"!
			// xpu_api->XPU_REG_RECV_ACK_COUNT_TOP_write( (((45+2+2)*10)<<16) | 10 );//add 2us for longer fir. BUT corrding to FPGA probing test, we do not need this
			// xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( 0 );
			// tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write(((10)*10)<<16);
		}
		else {
			//priv->slot_time = 9; //default slot time of OFDM PHY (OFDM by default means 5GHz)
			// xpu_api->XPU_REG_BAND_CHANNEL_write(BAND_5_8GHZ<<16);
			if (priv->band != BAND_5_8GHZ) {
				priv->band = BAND_5_8GHZ;
				xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16) );
			}
			// //xpu_api->XPU_REG_RECV_ACK_COUNT_TOP_write( (((51+2)*10)<<16) | 10 ); // because 5GHz needs longer SIFS (16 instead of 10), we need 58 instead of 48 for XPU low mac setting.  let's add 2us for those device that is really "slow"!
			// xpu_api->XPU_REG_RECV_ACK_COUNT_TOP_write( (((51+2+2)*10)<<16) | 10 );//add 2us for longer fir.  BUT corrding to FPGA probing test, we do not need this
			// //xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( 60*10 );
			// xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( 50*10 );// for longer fir we need this delay 1us shorter
			// tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write(((16)*10)<<16);
		}
		//printk("%s ad9361_rf_set_channel %dM rssi_correction %d\n", sdr_compatible_str,conf->chandef.chan->center_freq,priv->rssi_correction);
		// //-- use less
		//clk_prepare_enable(priv->ad9361_phy->clks[RX_RFPLL]);
		//printk("%s ad9361_rf_set_channel tune to %d read back %llu\n", sdr_compatible_str,conf->chandef.chan->center_freq,2*priv->ad9361_phy->state->current_rx_lo_freq);
		//ad9361_set_trx_clock_chain_default(priv->ad9361_phy);
		//printk("%s ad9361_rf_set_channel tune to %d read back %llu\n", sdr_compatible_str,conf->chandef.chan->center_freq,2*priv->ad9361_phy->state->current_rx_lo_freq);
	}
	printk("%s ad9361_rf_set_channel %dM rssi_correction %d (change flag %d)\n", sdr_compatible_str,conf->chandef.chan->center_freq,priv->rssi_correction,change_flag);
}

const struct openwifi_rf_ops ad9361_rf_ops = {
	.name		= "ad9361",
//	.init		= ad9361_rf_init,
//	.stop		= ad9361_rf_stop,
	.set_chan	= ad9361_rf_set_channel,
//	.calc_rssi	= ad9361_rf_calc_rssi,
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

	ring->stop_flag = 0;
	ring->bd_wr_idx = 0;
	ring->bd_rd_idx = 0;
	ring->bds = kmalloc(sizeof(struct openwifi_buffer_descriptor)*NUM_TX_BD,GFP_KERNEL);
	if (ring->bds==NULL) {
		printk("%s openwifi_init_tx_ring: WARNING Cannot allocate TX ring\n",sdr_compatible_str);
		return -ENOMEM;
	}

	for (i = 0; i < NUM_TX_BD; i++) {
		ring->bds[i].skb_linked=0; // for tx, skb is from upper layer
		//at first right after skb allocated, head, data, tail are the same.
		ring->bds[i].dma_mapping_addr = 0; // for tx, mapping is done after skb is received from upper layer in tx routine
	}

	return 0;
}

static void openwifi_free_tx_ring(struct openwifi_priv *priv, int ring_idx)
{
	struct openwifi_ring *ring = &(priv->tx_ring[ring_idx]);
	int i;

	ring->stop_flag = 0;
	ring->bd_wr_idx = 0;
	ring->bd_rd_idx = 0;
	for (i = 0; i < NUM_TX_BD; i++) {
		if (ring->bds[i].skb_linked == 0 && ring->bds[i].dma_mapping_addr == 0)
			continue;
		if (ring->bds[i].dma_mapping_addr != 0)
			dma_unmap_single(priv->tx_chan->device->dev, ring->bds[i].dma_mapping_addr,ring->bds[i].skb_linked->len, DMA_MEM_TO_DEV);
//		if (ring->bds[i].skb_linked!=NULL)
//			dev_kfree_skb(ring->bds[i].skb_linked); // only use dev_kfree_skb when there is exception
		if ( (ring->bds[i].dma_mapping_addr != 0 && ring->bds[i].skb_linked == 0) ||
		     (ring->bds[i].dma_mapping_addr == 0 && ring->bds[i].skb_linked != 0))
			printk("%s openwifi_free_tx_ring: WARNING ring %d i %d skb_linked %p dma_mapping_addr %08x\n", sdr_compatible_str, 
			ring_idx, i, (void*)(ring->bds[i].skb_linked), ring->bds[i].dma_mapping_addr);

		ring->bds[i].skb_linked=0;
		ring->bds[i].dma_mapping_addr = 0;
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
		(*((u32*)(pdata_tmp+0 ))) = 0;
		(*((u32*)(pdata_tmp+4 ))) = 0;
	}
	printk("%s openwifi_init_rx_ring: tsft_low and tsft_high are cleared!\n", sdr_compatible_str);

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

static irqreturn_t openwifi_rx_interrupt(int irq, void *dev_id)
{
	struct ieee80211_hw *dev = dev_id;
	struct openwifi_priv *priv = dev->priv;
	struct ieee80211_rx_status rx_status = {0};
	struct sk_buff *skb;
	struct ieee80211_hdr *hdr;
	u32 addr1_low32=0, addr2_low32=0, addr3_low32=0, len, rate_idx, tsft_low, tsft_high, loop_count=0, ht_flag, short_gi;//, fc_di;
	// u32 dma_driver_buf_idx_mod;
	u8 *pdata_tmp, fcs_ok;//, target_buf_idx;//, phy_rx_sn_hw;
	s8 signal;
	u16 rssi_val, addr1_high16=0, addr2_high16=0, addr3_high16=0, sc=0;
	bool content_ok = false, len_overflow = false;
	static u8 target_buf_idx_old = 0;

	spin_lock(&priv->lock);

	while(1) { // loop all rx buffers that have new rx packets
		pdata_tmp = priv->rx_cyclic_buf + target_buf_idx_old*RX_BD_BUF_SIZE; // our header insertion is at the beginning
		tsft_low =     (*((u32*)(pdata_tmp+0 )));
		tsft_high =    (*((u32*)(pdata_tmp+4 )));
		if ( tsft_low==0 && tsft_high==0 ) // no packet in the buffer
			break;

		rssi_val =     (*((u16*)(pdata_tmp+8 )));
		len =          (*((u16*)(pdata_tmp+12)));

		len_overflow = (len>(RX_BD_BUF_SIZE-16)?true:false);

		rate_idx =     (*((u16*)(pdata_tmp+14)));
		short_gi =     ((rate_idx&0x20)!=0);
		rate_idx =     (rate_idx&0xDF);

		fcs_ok = ( len_overflow?0:(*(( u8*)(pdata_tmp+16+len-1))) );

		//phy_rx_sn_hw = (fcs_ok&(NUM_RX_BD-1));
		// phy_rx_sn_hw = (fcs_ok&0x7f);//0x7f is FPGA limitation
		// dma_driver_buf_idx_mod = (state.residue&0x7f);
		fcs_ok = ((fcs_ok&0x80)!=0);
		ht_flag = ((rate_idx&0x10)!=0);

		if ( (len>=14 && (!len_overflow)) && (rate_idx>=8 && rate_idx<=23)) {
			// if ( phy_rx_sn_hw!=dma_driver_buf_idx_mod) {
			// 	printk("%s openwifi_rx_interrupt: WARNING sn %d next buf_idx %d!\n", sdr_compatible_str,phy_rx_sn_hw,dma_driver_buf_idx_mod);
			// }
			content_ok = true;
		} else {
			printk("%s openwifi_rx_interrupt: WARNING content!\n", sdr_compatible_str);
			content_ok = false;
		}

		rssi_val = (rssi_val>>1);
		if ( (rssi_val+128)<priv->rssi_correction )
			signal = -128;
		else
			signal = rssi_val - priv->rssi_correction;

		// fc_di =        (*((u32*)(pdata_tmp+16)));
		// addr1_high16 = (*((u16*)(pdata_tmp+16+4)));
		// addr1_low32  = (*((u32*)(pdata_tmp+16+4+2)));
		// addr2_high16 = (*((u16*)(pdata_tmp+16+6+4)));
		// addr2_low32  = (*((u32*)(pdata_tmp+16+6+4+2)));
		// addr3_high16 = (*((u16*)(pdata_tmp+16+12+4)));
		// addr3_low32  = (*((u32*)(pdata_tmp+16+12+4+2)));
		if ( (priv->drv_rx_reg_val[DRV_RX_REG_IDX_PRINT_CFG]&2) || ( (priv->drv_rx_reg_val[DRV_RX_REG_IDX_PRINT_CFG]&1) && fcs_ok==0 ) ) {
			hdr = (struct ieee80211_hdr *)(pdata_tmp+16);
			addr1_low32  = *((u32*)(hdr->addr1+2));
			addr1_high16 = *((u16*)(hdr->addr1));
			if (len>=20) {
				addr2_low32  = *((u32*)(hdr->addr2+2));
				addr2_high16 = *((u16*)(hdr->addr2));
			}
			if (len>=26) {
				addr3_low32  = *((u32*)(hdr->addr3+2));
				addr3_high16 = *((u16*)(hdr->addr3));
			}
			if (len>=28)
				sc = hdr->seq_ctrl;

			if ( addr1_low32!=0xffffffff || addr1_high16!=0xffff )
				printk("%s openwifi_rx_interrupt:%4dbytes ht%d %3dM FC%04x DI%04x addr1/2/3:%04x%08x/%04x%08x/%04x%08x SC%04x fcs%d buf_idx%d %ddBm\n", sdr_compatible_str,
					len, ht_flag, wifi_rate_table[rate_idx], hdr->frame_control, hdr->duration_id, 
					reverse16(addr1_high16), reverse32(addr1_low32), reverse16(addr2_high16), reverse32(addr2_low32), reverse16(addr3_high16), reverse32(addr3_low32), 
					sc, fcs_ok, target_buf_idx_old, signal);
		}
		
		// priv->phy_rx_sn_hw_old = phy_rx_sn_hw;
		if (content_ok) {
			skb = dev_alloc_skb(len);
			if (skb) {
				skb_put_data(skb,pdata_tmp+16,len);

				rx_status.antenna = 0;
				// def in ieee80211_rate openwifi_rates 0~11. 0~3 11b(1M~11M), 4~11 11a/g(6M~54M)
				rx_status.rate_idx = wifi_rate_table_mapping[rate_idx];
				rx_status.signal = signal;
				rx_status.freq = dev->conf.chandef.chan->center_freq;
				rx_status.band = dev->conf.chandef.chan->band;
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

				memcpy(IEEE80211_SKB_RXCB(skb), &rx_status, sizeof(rx_status)); // put rx_status into skb->cb, from now on skb->cb is not dma_dsts any more.
				ieee80211_rx_irqsafe(dev, skb); // call mac80211 function
			} else
				printk("%s openwifi_rx_interrupt: WARNING dev_alloc_skb failed!\n", sdr_compatible_str);
		}
		(*((u32*)(pdata_tmp+0 ))) = 0;
		(*((u32*)(pdata_tmp+4 ))) = 0; // clear the tsft_low and tsft_high to indicate the packet has been processed
		loop_count++;
		target_buf_idx_old=((target_buf_idx_old+1)&(NUM_RX_BD-1)); 
	}

	if ( loop_count!=1 && (priv->drv_rx_reg_val[DRV_RX_REG_IDX_PRINT_CFG]&1) )
		printk("%s openwifi_rx_interrupt: WARNING loop_count %d\n", sdr_compatible_str,loop_count);
	
// openwifi_rx_interrupt_out:
	spin_unlock(&priv->lock);
	return IRQ_HANDLED;
}

static irqreturn_t openwifi_tx_interrupt(int irq, void *dev_id)
{
	struct ieee80211_hw *dev = dev_id;
	struct openwifi_priv *priv = dev->priv;
	struct openwifi_ring *ring;
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;
	u32 reg_val, hw_queue_len, prio, queue_idx, dma_fifo_no_room_flag, num_slot_random, cw, loop_count=0;//, i;
	u8 tx_result_report;
	// u16 prio_rd_idx_store[64]={0};

	spin_lock(&priv->lock);

	while(1) { // loop all packets that have been sent by FPGA
		reg_val = tx_intf_api->TX_INTF_REG_PKT_INFO_read();
		if (reg_val!=0xFFFFFFFF) {
			prio = ((0x7FFFF & reg_val)>>(5+NUM_BIT_MAX_PHY_TX_SN+NUM_BIT_MAX_NUM_HW_QUEUE));
			cw = ((0xF0000000 & reg_val) >> 28);
			num_slot_random = ((0xFF80000 &reg_val)>>(2+5+NUM_BIT_MAX_PHY_TX_SN+NUM_BIT_MAX_NUM_HW_QUEUE));
			if(cw > 10) {
				cw = 10 ;
				num_slot_random += 512 ; 
			}
			
			ring = &(priv->tx_ring[prio]);
			ring->bd_rd_idx = ((reg_val>>5)&MAX_PHY_TX_SN);
			skb = ring->bds[ring->bd_rd_idx].skb_linked;

			dma_unmap_single(priv->tx_chan->device->dev,ring->bds[ring->bd_rd_idx].dma_mapping_addr,
					skb->len, DMA_MEM_TO_DEV);

			if ( ring->stop_flag == 1) {
				// Wake up Linux queue if FPGA and driver ring have room
				queue_idx = ((reg_val>>(5+NUM_BIT_MAX_PHY_TX_SN))&(MAX_NUM_HW_QUEUE-1));
				dma_fifo_no_room_flag = tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read();
				hw_queue_len = tx_intf_api->TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read();

				// printk("%s openwifi_tx_interrupt: WARNING loop %d prio %d queue %d no room flag %x hw queue len %08x wr %d rd %d call %d\n", sdr_compatible_str,
				// loop_count, prio, queue_idx, dma_fifo_no_room_flag, hw_queue_len, ring->bd_wr_idx, ring->bd_rd_idx, priv->call_counter);

				if ( ((dma_fifo_no_room_flag>>queue_idx)&1)==0 && (NUM_TX_BD-((hw_queue_len>>(queue_idx*8))&0xFF))>=RING_ROOM_THRESHOLD ) {
					// printk("%s openwifi_tx_interrupt: WARNING ieee80211_wake_queue loop %d call %d\n", sdr_compatible_str, loop_count, priv->call_counter);
					printk("%s openwifi_tx_interrupt: WARNING ieee80211_wake_queue prio %d queue %d no room flag %x hw queue len %08x wr %d rd %d\n", sdr_compatible_str,
					prio, queue_idx, dma_fifo_no_room_flag, hw_queue_len, ring->bd_wr_idx, ring->bd_rd_idx);
					ieee80211_wake_queue(dev, prio);
					ring->stop_flag = 0;
				}
			}

			if ( (*(u32*)(&(skb->data[4]))) || ((*(u32*)(&(skb->data[12])))&0xFFFF0000) ) {
				printk("%s openwifi_tx_interrupt: WARNING %08x %08x %08x %08x\n", sdr_compatible_str, *(u32*)(&(skb->data[12])), *(u32*)(&(skb->data[8])), *(u32*)(&(skb->data[4])), *(u32*)(&(skb->data[0])));
				continue;
			}

			skb_pull(skb, LEN_PHY_HEADER);
			//skb_trim(skb, num_byte_pad_skb);
			info = IEEE80211_SKB_CB(skb);
			ieee80211_tx_info_clear_status(info);

			tx_result_report = (reg_val&0x1F);
			if ( !(info->flags & IEEE80211_TX_CTL_NO_ACK) ) {
				if ((tx_result_report&0x10)==0)
					info->flags |= IEEE80211_TX_STAT_ACK;

				// printk("%s openwifi_tx_interrupt: rate&try: %d %d %03x; %d %d %03x; %d %d %03x; %d %d %03x\n", sdr_compatible_str,
				// 	info->status.rates[0].idx,info->status.rates[0].count,info->status.rates[0].flags,
				// 	info->status.rates[1].idx,info->status.rates[1].count,info->status.rates[1].flags,
				// 	info->status.rates[2].idx,info->status.rates[2].count,info->status.rates[2].flags,
				// 	info->status.rates[3].idx,info->status.rates[3].count,info->status.rates[3].flags);
			}

			info->status.rates[0].count = (tx_result_report&0xF) + 1; //according to our test, the 1st rate is the most important. we only do retry on the 1st rate
			info->status.rates[1].idx = -1;
			info->status.rates[2].idx = -1;
			info->status.rates[3].idx = -1;//in mac80211.h: #define IEEE80211_TX_MAX_RATES	4
			
			if ( (tx_result_report&0x10) && ((priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG])&1) )
				printk("%s openwifi_tx_interrupt: WARNING tx_result %02x prio%d wr%d rd%d\n", sdr_compatible_str, tx_result_report, prio, ring->bd_wr_idx, ring->bd_rd_idx);
			if ( (!(info->flags & IEEE80211_TX_CTL_NO_ACK)) && ((priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG])&2) )
				printk("%s openwifi_tx_interrupt: tx_result %02x prio%d wr%d rd%d num_rand_slot %d cw %d \n", sdr_compatible_str, tx_result_report, prio, ring->bd_wr_idx, ring->bd_rd_idx, num_slot_random,cw);

			ieee80211_tx_status_irqsafe(dev, skb);
			
			loop_count++;
			
			// printk("%s openwifi_tx_interrupt: loop %d prio %d rd %d\n", sdr_compatible_str, loop_count, prio, ring->bd_rd_idx);

		} else
			break;
	}
	if ( loop_count!=1 && ((priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG])&1) )
		printk("%s openwifi_tx_interrupt: WARNING loop_count %d\n", sdr_compatible_str, loop_count);

	spin_unlock(&priv->lock);
	return IRQ_HANDLED;
}

u32 gen_parity(u32 v){
	v ^= v >> 1;
	v ^= v >> 2;
	v = (v & 0x11111111U) * 0x11111111U;
	return (v >> 28) & 1;
}

u8 gen_ht_sig_crc(u64 m)
{
	u8 i, temp, c[8] = {1, 1, 1, 1, 1, 1, 1, 1}, ht_sig_crc;

	for (i = 0; i < 34; i++)
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
	ht_sig_crc = ((~c[7] & 0x01) << 0) | ((~c[6] & 0x01) << 1) | ((~c[5] & 0x01) << 2) | ((~c[4] & 0x01) << 3) | ((~c[3] & 0x01) << 4) | ((~c[2] & 0x01) << 5) | ((~c[1] & 0x01) << 6) | ((~c[0] & 0x01) << 7);

	return ht_sig_crc;
}

u32 calc_phy_header(u8 rate_hw_value, bool use_ht_rate, bool use_short_gi, u32 len, u8 *bytes){
	//u32 signal_word = 0 ;
	u8  SIG_RATE = 0, HT_SIG_RATE;
	u8	len_2to0, len_10to3, len_msb,b0,b1,b2, header_parity ;
	u32 l_len, ht_len, ht_sig1, ht_sig2;

	// printk("rate_hw_value=%u\tuse_ht_rate=%u\tuse_short_gi=%u\tlen=%u\n", rate_hw_value, use_ht_rate, use_short_gi, len);

	// HT-mixed mode ht signal

	if(use_ht_rate)
	{
		SIG_RATE = wifi_mcs_table_11b_force_up[4];
		HT_SIG_RATE = rate_hw_value;
		l_len  = 24 * len / wifi_n_dbps_ht_table[rate_hw_value];
		ht_len = len;
	}
	else
	{
		// rate_hw_value = (rate_hw_value<=4?0:(rate_hw_value-4));
		// SIG_RATE = wifi_mcs_table_phy_tx[rate_hw_value];
		SIG_RATE = wifi_mcs_table_11b_force_up[rate_hw_value];
		l_len = len;
	}

	len_2to0 = l_len & 0x07 ;
	len_10to3 = (l_len >> 3 ) & 0xFF ;
	len_msb = (l_len >> 11) & 0x01 ;

	b0=SIG_RATE | (len_2to0 << 5) ;
	b1 = len_10to3 ;
	header_parity = gen_parity((len_msb << 16)| (b1<<8) | b0) ;
	b2 = ( len_msb | (header_parity << 1) ) ;

	memset(bytes,0,16);
	bytes[0] = b0 ;
	bytes[1] = b1 ; 
    bytes[2] = b2;

	// HT-mixed mode signal
	if(use_ht_rate)
	{
		ht_sig1 = (HT_SIG_RATE & 0x7F) | ((ht_len << 8) & 0xFFFF00);
		ht_sig2 = 0x04 | (use_short_gi << 7);
		ht_sig2 = ht_sig2 | (gen_ht_sig_crc(ht_sig1 | ht_sig2 << 24) << 10);

	    bytes[3]  = 1;
	    bytes[8]  = (ht_sig1 & 0xFF);
	    bytes[9]  = (ht_sig1 >> 8)  & 0xFF;
	    bytes[10] = (ht_sig1 >> 16) & 0xFF;
	    bytes[11] = (ht_sig2 & 0xFF);
	    bytes[12] = (ht_sig2 >> 8)  & 0xFF;
	    bytes[13] = (ht_sig2 >> 16) & 0xFF;

		return(HT_SIG_RATE);
	}
	else
	{
		//signal_word = b0+(b1<<8)+(b2<<16) ;
		//return signal_word;
		return(SIG_RATE);
	}
}

static inline struct gpio_led_data * //please align with the implementation in leds-gpio.c
			cdev_to_gpio_led_data(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct gpio_led_data, cdev);
}

static void openwifi_tx(struct ieee80211_hw *dev,
		       struct ieee80211_tx_control *control,
		       struct sk_buff *skb)
{
	struct openwifi_priv *priv = dev->priv;
	unsigned long flags;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
	struct openwifi_ring *ring;
	dma_addr_t dma_mapping_addr;
	unsigned int prio, i;
	u32 num_dma_symbol, len_mac_pdu, num_dma_byte, len_phy_packet, num_byte_pad;
	u32 rate_signal_value,rate_hw_value,ack_flag;
	u32 pkt_need_ack, addr1_low32=0, addr2_low32=0, addr3_low32=0, queue_idx=2, dma_reg, cts_reg;//, openofdm_state_history;
	u16 addr1_high16=0, addr2_high16=0, addr3_high16=0, sc=0, cts_duration=0, cts_rate_hw_value = 0, cts_rate_signal_value=0, sifs, ack_duration=0, traffic_pkt_duration;
	u8 fc_flag,fc_type,fc_subtype,retry_limit_raw,*dma_buf,retry_limit_hw_value,rc_flags;
	bool use_rts_cts, use_cts_protect, use_ht_rate=false, use_short_gi, addr_flag, cts_use_traffic_rate=false, force_use_cts_protect=false;
	__le16 frame_control,duration_id;
	u32 dma_fifo_no_room_flag, hw_queue_len;
	enum dma_status status;
	// static bool led_status=0;
	// struct gpio_led_data *led_dat = cdev_to_gpio_led_data(priv->led[3]);

	// if ( (priv->phy_tx_sn&7) ==0 ) {
	// 	openofdm_state_history = openofdm_rx_api->OPENOFDM_RX_REG_STATE_HISTORY_read();
	// 	if (openofdm_state_history!=openofdm_state_history_old){
	// 		led_status = (~led_status);
	// 		openofdm_state_history_old = openofdm_state_history;
	// 		gpiod_set_value(led_dat->gpiod, led_status);
	// 	}
	// }

	if (test_mode==1){
		printk("%s openwifi_tx: WARNING test_mode==1\n", sdr_compatible_str);
		goto openwifi_tx_early_out;
	}

	if (skb->data_len>0) {// more data are not in linear data area skb->data
		printk("%s openwifi_tx: WARNING skb->data_len>0\n", sdr_compatible_str);
		goto openwifi_tx_early_out;
	}

	len_mac_pdu = skb->len;
	len_phy_packet = len_mac_pdu + LEN_PHY_HEADER;
	num_dma_symbol = (len_phy_packet>>TX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS) + ((len_phy_packet&(TX_INTF_NUM_BYTE_PER_DMA_SYMBOL-1))!=0);

	// get Linux priority/queue setting info and target mac address
	prio = skb_get_queue_mapping(skb);
	addr1_low32  = *((u32*)(hdr->addr1+2));
	ring = &(priv->tx_ring[prio]);

	// -------------- DO your idea here! Map Linux/SW "prio" to hardware "queue_idx" -----------
	if (priv->slice_idx == 0xFFFFFFFF) {// use Linux default prio setting, if there isn't any slice config
		queue_idx = prio;
	} else {// customized prio to queue_idx mapping
		//if (fc_type==2 && fc_subtype==0 && (!addr_flag)) { // for unicast data packet only
		// check current packet belonging to which slice/hw-queue
			for (i=0; i<MAX_NUM_HW_QUEUE; i++) {
				if ( priv->dest_mac_addr_queue_map[i] == addr1_low32 ) {
					break;
				}
			}
		//}
		queue_idx = (i>=MAX_NUM_HW_QUEUE?2:i); // if no address is hit, use FPGA queue 2. because the queue 2 is the longest.
	}
	// -------------------- end of Map Linux/SW "prio" to hardware "queue_idx" ------------------

	// check whether the packet is bigger than DMA buffer size
	num_dma_byte = (num_dma_symbol<<TX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS);
	if (num_dma_byte > TX_BD_BUF_SIZE) {
		// dev_err(priv->tx_chan->device->dev, "sdr,sdr openwifi_tx: WARNING num_dma_byte > TX_BD_BUF_SIZE\n");
		printk("%s openwifi_tx: WARNING sn %d num_dma_byte > TX_BD_BUF_SIZE\n", sdr_compatible_str, ring->bd_wr_idx);
		goto openwifi_tx_early_out;
	}
	num_byte_pad = num_dma_byte-len_phy_packet;

	// get other info from packet header
	addr1_high16 = *((u16*)(hdr->addr1));
	if (len_mac_pdu>=20) {
		addr2_low32  = *((u32*)(hdr->addr2+2));
		addr2_high16 = *((u16*)(hdr->addr2));
	}
	if (len_mac_pdu>=26) {
		addr3_low32  = *((u32*)(hdr->addr3+2));
		addr3_high16 = *((u16*)(hdr->addr3));
	}

	duration_id = hdr->duration_id;
	frame_control=hdr->frame_control;
	ack_flag = (info->flags&IEEE80211_TX_CTL_NO_ACK);
	fc_type = ((frame_control)>>2)&3;
	fc_subtype = ((frame_control)>>4)&0xf;
	fc_flag = ( fc_type==2 || fc_type==0 || (fc_type==1 && (fc_subtype==8 || fc_subtype==9 || fc_subtype==10) ) );
	//if it is broadcasting or multicasting addr
	addr_flag = ( (addr1_low32==0 && addr1_high16==0) || 
	              (addr1_low32==0xFFFFFFFF && addr1_high16==0xFFFF) ||
				  (addr1_high16==0x3333) ||
				  (addr1_high16==0x0001 && hdr->addr1[2]==0x5E)  );
	if ( fc_flag && ( !addr_flag ) && (!ack_flag) ) { // unicast data frame
		pkt_need_ack = 1; //FPGA need to wait ACK after this pkt sent
	} else {
		pkt_need_ack = 0;
	}

	// get Linux rate (MCS) setting
	rate_hw_value = ieee80211_get_tx_rate(dev, info)->hw_value;
	//rate_hw_value = 10; //4:6M, 5:9M, 6:12M, 7:18M, 8:24M, 9:36M, 10:48M, 11:54M
	if (priv->drv_tx_reg_val[DRV_TX_REG_IDX_RATE]>0 && fc_type==2 && (!addr_flag)) //rate override command
		rate_hw_value = priv->drv_tx_reg_val[DRV_TX_REG_IDX_RATE];

	retry_limit_raw = info->control.rates[0].count;

	rc_flags = info->control.rates[0].flags;
	use_rts_cts = ((rc_flags&IEEE80211_TX_RC_USE_RTS_CTS)!=0);
	use_cts_protect = ((rc_flags&IEEE80211_TX_RC_USE_CTS_PROTECT)!=0);
	use_ht_rate = ((rc_flags&IEEE80211_TX_RC_MCS)!=0);
	use_short_gi = ((rc_flags&IEEE80211_TX_RC_SHORT_GI)!=0);

	if (use_rts_cts)
		printk("%s openwifi_tx: WARNING sn %d use_rts_cts is not supported!\n", sdr_compatible_str, ring->bd_wr_idx);

	if (use_cts_protect) {
		cts_rate_hw_value = ieee80211_get_rts_cts_rate(dev, info)->hw_value;
		cts_duration = le16_to_cpu(ieee80211_ctstoself_duration(dev,info->control.vif,len_mac_pdu,info));
	} else if (force_use_cts_protect) { // could override mac80211 setting here.
		cts_rate_hw_value = 4; //wifi_mcs_table_11b_force_up[] translate it to 1011(6M)
		sifs = (priv->actual_rx_lo<2500?10:16);
		if (pkt_need_ack)
			ack_duration = 44;//assume the ack we wait use 6Mbps: 4*ceil((22+14*8)/24) + 20(preamble+SIGNAL)
		traffic_pkt_duration = 20 + 4*(((22+len_mac_pdu*8)/wifi_n_dbps_table[rate_hw_value])+1);
		cts_duration = traffic_pkt_duration + sifs + pkt_need_ack*(sifs+ack_duration);
	}

// this is 11b stuff
//	if (info->flags&IEEE80211_TX_RC_USE_SHORT_PREAMBLE)
//		printk("%s openwifi_tx: WARNING IEEE80211_TX_RC_USE_SHORT_PREAMBLE\n", sdr_compatible_str);

	if (len_mac_pdu>=28) {
		if (info->flags & IEEE80211_TX_CTL_ASSIGN_SEQ) {
			if (info->flags & IEEE80211_TX_CTL_FIRST_FRAGMENT)
				priv->seqno += 0x10;
			hdr->seq_ctrl &= cpu_to_le16(IEEE80211_SCTL_FRAG);
			hdr->seq_ctrl |= cpu_to_le16(priv->seqno);
		}
		sc = hdr->seq_ctrl;
	}

	if ( (!addr_flag) && (priv->drv_tx_reg_val[DRV_TX_REG_IDX_PRINT_CFG]&2) ) 
		printk("%s openwifi_tx: %4dbytes ht%d %3dM FC%04x DI%04x addr1/2/3:%04x%08x/%04x%08x/%04x%08x SC%04x flag%08x retr%d ack%d prio%d q%d wr%d rd%d\n", sdr_compatible_str,
			len_mac_pdu, (use_ht_rate == false ? 0 : 1), (use_ht_rate == false ? wifi_rate_all[rate_hw_value] : wifi_rate_all[rate_hw_value + 12]),frame_control,duration_id, 
			reverse16(addr1_high16), reverse32(addr1_low32), reverse16(addr2_high16), reverse32(addr2_low32), reverse16(addr3_high16), reverse32(addr3_low32),
			sc, info->flags, retry_limit_raw, pkt_need_ack, prio, queue_idx,
			// use_rts_cts,use_cts_protect|force_use_cts_protect,wifi_rate_all[cts_rate_hw_value],cts_duration,
			ring->bd_wr_idx,ring->bd_rd_idx);

		// printk("%s openwifi_tx: rate&try: %d %d %03x; %d %d %03x; %d %d %03x; %d %d %03x\n", sdr_compatible_str,
		// 	info->status.rates[0].idx,info->status.rates[0].count,info->status.rates[0].flags,
		// 	info->status.rates[1].idx,info->status.rates[1].count,info->status.rates[1].flags,
		// 	info->status.rates[2].idx,info->status.rates[2].count,info->status.rates[2].flags,
		// 	info->status.rates[3].idx,info->status.rates[3].count,info->status.rates[3].flags);

	// -----------end of preprocess some info from header and skb----------------

	// /* HW will perform RTS-CTS when only RTS flags is set.
	//  * HW will perform CTS-to-self when both RTS and CTS flags are set.
	//  * RTS rate and RTS duration will be used also for CTS-to-self.
	//  */
	// if (rc_flags & IEEE80211_TX_RC_USE_RTS_CTS) {
	// 	tx_flags |= ieee80211_get_rts_cts_rate(dev, info)->hw_value << 19;
	// 	rts_duration = ieee80211_rts_duration(dev, priv->vif[0], // assume all vif have the same config
	// 					len_mac_pdu, info);
	// 	printk("%s openwifi_tx: rc_flags & IEEE80211_TX_RC_USE_RTS_CTS\n", sdr_compatible_str);
	// } else if (rc_flags & IEEE80211_TX_RC_USE_CTS_PROTECT) {
	// 	tx_flags |= ieee80211_get_rts_cts_rate(dev, info)->hw_value << 19;
	// 	rts_duration = ieee80211_ctstoself_duration(dev, priv->vif[0], // assume all vif have the same config
	// 					len_mac_pdu, info);
	// 	printk("%s openwifi_tx: rc_flags & IEEE80211_TX_RC_USE_CTS_PROTECT\n", sdr_compatible_str);
	// }

	// when skb does not have enough headroom, skb_push will cause kernel panic. headroom needs to be extended if necessary
	if (skb_headroom(skb)<LEN_PHY_HEADER) {
		struct sk_buff *skb_new; // in case original skb headroom is not enough to host phy header needed by FPGA IP core
		printk("%s openwifi_tx: WARNING sn %d skb_headroom(skb)<LEN_PHY_HEADER\n", sdr_compatible_str, ring->bd_wr_idx);
		if ((skb_new = skb_realloc_headroom(skb, LEN_PHY_HEADER)) == NULL) {
			printk("%s openwifi_tx: WARNING sn %d skb_realloc_headroom failed!\n", sdr_compatible_str, ring->bd_wr_idx);
			goto openwifi_tx_early_out;
		}
		if (skb->sk != NULL)
			skb_set_owner_w(skb_new, skb->sk);
		dev_kfree_skb(skb);
		skb = skb_new;
	}
	
	skb_push( skb, LEN_PHY_HEADER );
	rate_signal_value = calc_phy_header(rate_hw_value, use_ht_rate, use_short_gi, len_mac_pdu+LEN_PHY_CRC, skb->data); //fill the phy header

	//make sure dma length is integer times of DDC_NUM_BYTE_PER_DMA_SYMBOL
	if (skb_tailroom(skb)<num_byte_pad) {
		printk("%s openwifi_tx: WARNING sn %d skb_tailroom(skb)<num_byte_pad!\n", sdr_compatible_str, ring->bd_wr_idx);
		// skb_pull(skb, LEN_PHY_HEADER);
		goto openwifi_tx_early_out;
	}
	skb_put( skb, num_byte_pad );

	retry_limit_hw_value = ( retry_limit_raw==0?0:((retry_limit_raw - 1)&0xF) );
	dma_buf = skb->data;

	cts_rate_signal_value = wifi_mcs_table_11b_force_up[cts_rate_hw_value];
	cts_reg = (((use_cts_protect|force_use_cts_protect)<<31)|(cts_use_traffic_rate<<30)|(cts_duration<<8)|(cts_rate_signal_value<<4)|rate_signal_value);
	dma_reg = ( (( ((prio<<(NUM_BIT_MAX_NUM_HW_QUEUE+NUM_BIT_MAX_PHY_TX_SN))|(ring->bd_wr_idx<<NUM_BIT_MAX_NUM_HW_QUEUE)|queue_idx) )<<18)|(retry_limit_hw_value<<14)|(pkt_need_ack<<13)|num_dma_symbol );

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
	if ( ((dma_fifo_no_room_flag>>queue_idx)&1) || ((NUM_TX_BD-((hw_queue_len>>(queue_idx*8))&0xFF))<RING_ROOM_THRESHOLD)  || ring->stop_flag==1 ) {
		ieee80211_stop_queue(dev, prio); // here we should stop those prio related to the queue idx flag set in TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read
		printk("%s openwifi_tx: WARNING ieee80211_stop_queue prio %d queue %d no room flag %x hw queue len %08x request %d wr %d rd %d\n", sdr_compatible_str,
		prio, queue_idx, dma_fifo_no_room_flag, hw_queue_len, num_dma_symbol, ring->bd_wr_idx, ring->bd_rd_idx);
		ring->stop_flag = 1;
		goto openwifi_tx_early_out_after_lock;
	}
	// --------end of check whether FPGA fifo (queue_idx) has enough room------------

	status = dma_async_is_tx_complete(priv->tx_chan, priv->tx_cookie, NULL, NULL);
	if (status!=DMA_COMPLETE) {
		printk("%s openwifi_tx: WARNING status!=DMA_COMPLETE\n", sdr_compatible_str);
		goto openwifi_tx_early_out_after_lock;
	}

	if ( (*(u32*)(&(skb->data[4]))) || ((*(u32*)(&(skb->data[12])))&0xFFFF0000) ) {
		printk("%s openwifi_tx: WARNING 1 %d %08x %08x %08x %08x\n", sdr_compatible_str, num_byte_pad, *(u32*)(&(skb->data[12])), *(u32*)(&(skb->data[8])), *(u32*)(&(skb->data[4])), *(u32*)(&(skb->data[0])));
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
	tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write(dma_reg);
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
	ring->bds[ring->bd_wr_idx].skb_linked = skb;
	ring->bds[ring->bd_wr_idx].dma_mapping_addr = dma_mapping_addr;

	ring->bd_wr_idx = ((ring->bd_wr_idx+1)&(NUM_TX_BD-1));

	dma_async_issue_pending(priv->tx_chan);

	if ( (*(u32*)(&(skb->data[4]))) || ((*(u32*)(&(skb->data[12])))&0xFFFF0000) )
		printk("%s openwifi_tx: WARNING 2 %08x %08x %08x %08x\n", sdr_compatible_str, *(u32*)(&(skb->data[12])), *(u32*)(&(skb->data[8])), *(u32*)(&(skb->data[4])), *(u32*)(&(skb->data[0])));

	spin_unlock_irqrestore(&priv->lock, flags);

	return;

openwifi_tx_after_dma_mapping:
	dma_unmap_single(priv->tx_chan->device->dev, dma_mapping_addr, num_dma_byte, DMA_MEM_TO_DEV);

openwifi_tx_early_out_after_lock:
	// skb_pull(skb, LEN_PHY_HEADER);
	dev_kfree_skb(skb);
	spin_unlock_irqrestore(&priv->lock, flags);
	// printk("%s openwifi_tx: WARNING openwifi_tx_after_dma_mapping phy_tx_sn %d queue %d\n", sdr_compatible_str,priv->phy_tx_sn,queue_idx);
	return;

openwifi_tx_early_out:
	dev_kfree_skb(skb);
	// printk("%s openwifi_tx: WARNING openwifi_tx_early_out phy_tx_sn %d queue %d\n", sdr_compatible_str,priv->phy_tx_sn,queue_idx);
}

static int openwifi_start(struct ieee80211_hw *dev)
{
	struct openwifi_priv *priv = dev->priv;
	int ret, i, rssi_half_db_offset, agc_gain_delay;//rssi_half_db_th, 
	u32 reg;

	for (i=0; i<MAX_NUM_VIF; i++) {
		priv->vif[i] = NULL;
	}

	memset(priv->drv_tx_reg_val, 0, sizeof(priv->drv_tx_reg_val));
	memset(priv->drv_rx_reg_val, 0, sizeof(priv->drv_rx_reg_val));
	memset(priv->drv_xpu_reg_val, 0, sizeof(priv->drv_xpu_reg_val));
	priv->drv_xpu_reg_val[DRV_XPU_REG_IDX_GIT_REV] = GIT_REV;

	//turn on radio
	if (priv->tx_intf_cfg == TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1) {
		ad9361_set_tx_atten(priv->ad9361_phy, AD9361_RADIO_ON_TX_ATT, false, true, true); // AD9361_RADIO_ON_TX_ATT 3000 means 3dB, 0 means 0dB
		reg = ad9361_get_tx_atten(priv->ad9361_phy, 2);
	} else {
		ad9361_set_tx_atten(priv->ad9361_phy, AD9361_RADIO_ON_TX_ATT, true, false, true); // AD9361_RADIO_ON_TX_ATT 3000 means 3dB, 0 means 0dB
		reg = ad9361_get_tx_atten(priv->ad9361_phy, 1);
	}
	if (reg == AD9361_RADIO_ON_TX_ATT) {
		priv->rfkill_off = 1;// 0 off, 1 on
		printk("%s openwifi_start: rfkill radio on\n",sdr_compatible_str);
	}
	else
		printk("%s openwifi_start: WARNING rfkill radio on failed. tx att read %d require %d\n",sdr_compatible_str, reg, AD9361_RADIO_ON_TX_ATT);

	if (priv->rx_intf_cfg == RX_INTF_BW_20MHZ_AT_0MHZ_ANT0)
		priv->ctrl_out.index=0x16;
	else
		priv->ctrl_out.index=0x17;

	ret = ad9361_ctrl_outs_setup(priv->ad9361_phy, &(priv->ctrl_out));
	if (ret < 0) {
		printk("%s openwifi_start: WARNING ad9361_ctrl_outs_setup %d\n",sdr_compatible_str, ret);
	} else {
		printk("%s openwifi_start: ad9361_ctrl_outs_setup en_mask 0x%02x index 0x%02x\n",sdr_compatible_str, priv->ctrl_out.en_mask, priv->ctrl_out.index);
	}

	priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
	priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];

	rx_intf_api->hw_init(priv->rx_intf_cfg,8,8);
	tx_intf_api->hw_init(priv->tx_intf_cfg,8,8,priv->fpga_type);
	openofdm_tx_api->hw_init(priv->openofdm_tx_cfg);
	openofdm_rx_api->hw_init(priv->openofdm_rx_cfg);
	xpu_api->hw_init(priv->xpu_cfg);

	agc_gain_delay = 50; //samples
	rssi_half_db_offset = 150; // to be consistent 
	xpu_api->XPU_REG_RSSI_DB_CFG_write(0x80000000|((rssi_half_db_offset<<16)|agc_gain_delay) );
	xpu_api->XPU_REG_RSSI_DB_CFG_write((~0x80000000)&((rssi_half_db_offset<<16)|agc_gain_delay) );
	
	openofdm_rx_api->OPENOFDM_RX_REG_POWER_THRES_write(0);
	// rssi_half_db_th = 87<<1; // -62dBm // will setup in runtime in _rf_set_channel
	// xpu_api->XPU_REG_LBT_TH_write(rssi_half_db_th); // set IQ rssi th step .5dB to xxx and enable it
	xpu_api->XPU_REG_FORCE_IDLE_MISC_write(75); //control the duration to force ch_idle after decoding a packet due to imperfection of agc and signals

	//xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( ((40)<<16)|0 );//high 16bit 5GHz; low 16 bit 2.4GHz (Attention, current tx core has around 1.19us starting delay that makes the ack fall behind 10us SIFS in 2.4GHz! Need to improve TX in 2.4GHz!)
	//xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( ((51)<<16)|0 );//now our tx send out I/Q immediately
	xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( ((51+23)<<16)|(0+23) );//we have more time when we use FIR in AD9361

	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP0_write( (1<<31) | (((45+2+2)*10 + 15)<<16) | 10 );//2.4GHz. extra 300 clocks are needed when rx core fall into fake ht detection phase (rx mcs 6M)
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write( (1<<31) | (((51+2+2)*10 + 15)<<16) | 10 );//5GHz. extra 300 clocks are needed when rx core fall into fake ht detection phase (rx mcs 6M)

	tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write( ((16*10)<<16)|(10*10) );//high 16bit 5GHz; low 16 bit 2.4GHz. counter speed 10MHz is assumed
	
	// //xpu_api->XPU_REG_BB_RF_DELAY_write(51); // fine tuned value at 0.005us. old: dac-->ant port: 0.6us, 57 taps fir at 40MHz: 1.425us; round trip: 2*(0.6+1.425)=4.05us; 4.05*10=41
	// xpu_api->XPU_REG_BB_RF_DELAY_write(47);//add .5us for slightly longer fir -- already in xpu.c
	xpu_api->XPU_REG_MAC_ADDR_write(priv->mac_addr);

	// setup time schedule of 4 slices
	// slice 0
	xpu_api->XPU_REG_SLICE_COUNT_TOTAL_write(50000-1); // total 50ms
	xpu_api->XPU_REG_SLICE_COUNT_START_write(0); //start 0ms
	xpu_api->XPU_REG_SLICE_COUNT_END_write(50000-1); //end 50ms

	// slice 1
	xpu_api->XPU_REG_SLICE_COUNT_TOTAL_write((1<<20)|(50000-1)); // total 50ms
	xpu_api->XPU_REG_SLICE_COUNT_START_write((1<<20)|(0)); //start 0ms
	//xpu_api->XPU_REG_SLICE_COUNT_END_write((1<<20)|(20000-1)); //end 20ms
	xpu_api->XPU_REG_SLICE_COUNT_END_write((1<<20)|(50000-1)); //end 20ms

	// slice 2
	xpu_api->XPU_REG_SLICE_COUNT_TOTAL_write((2<<20)|(50000-1)); // total 50ms
	//xpu_api->XPU_REG_SLICE_COUNT_START_write((2<<20)|(20000)); //start 20ms
	xpu_api->XPU_REG_SLICE_COUNT_START_write((2<<20)|(0)); //start 20ms
	//xpu_api->XPU_REG_SLICE_COUNT_END_write((2<<20)|(40000-1)); //end 20ms
	xpu_api->XPU_REG_SLICE_COUNT_END_write((2<<20)|(50000-1)); //end 20ms

	// slice 3
	xpu_api->XPU_REG_SLICE_COUNT_TOTAL_write((3<<20)|(50000-1)); // total 50ms
	//xpu_api->XPU_REG_SLICE_COUNT_START_write((3<<20)|(40000)); //start 40ms
	xpu_api->XPU_REG_SLICE_COUNT_START_write((3<<20)|(0)); //start 40ms
	//xpu_api->XPU_REG_SLICE_COUNT_END_write((3<<20)|(50000-1)); //end 20ms
	xpu_api->XPU_REG_SLICE_COUNT_END_write((3<<20)|(50000-1)); //end 20ms

	// all slice sync rest
	xpu_api->XPU_REG_MULTI_RST_write(1<<7); //bit7 reset the counter for all queues at the same time
	xpu_api->XPU_REG_MULTI_RST_write(0<<7); 

	//xpu_api->XPU_REG_MAC_ADDR_HIGH_write( (*( (u16*)(priv->mac_addr + 4) )) );
	printk("%s openwifi_start: rx_intf_cfg %d openofdm_rx_cfg %d tx_intf_cfg %d openofdm_tx_cfg %d\n",sdr_compatible_str, priv->rx_intf_cfg, priv->openofdm_rx_cfg, priv->tx_intf_cfg, priv->openofdm_tx_cfg);
	printk("%s openwifi_start: rx_freq_offset_to_lo_MHz %d tx_freq_offset_to_lo_MHz %d\n",sdr_compatible_str, priv->rx_freq_offset_to_lo_MHz, priv->tx_freq_offset_to_lo_MHz);

	tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x30004); //disable tx interrupt
	rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_write(0x100); // disable rx interrupt by interrupt test mode
	rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(1); // hold M AXIS in reset status

	if (test_mode==1) {
		printk("%s openwifi_start: test_mode==1\n",sdr_compatible_str);
		goto normal_out;
	}

	priv->rx_chan = dma_request_slave_channel(&(priv->pdev->dev), "rx_dma_s2mm");
	if (IS_ERR(priv->rx_chan)) {
		ret = PTR_ERR(priv->rx_chan);
		pr_err("%s openwifi_start: No Rx channel %d\n",sdr_compatible_str,ret);
		goto err_dma;
	}

	priv->tx_chan = dma_request_slave_channel(&(priv->pdev->dev), "tx_dma_mm2s");
	if (IS_ERR(priv->tx_chan)) {
		ret = PTR_ERR(priv->tx_chan);
		pr_err("%s openwifi_start: No Tx channel %d\n",sdr_compatible_str,ret);
		goto err_dma;
	}
	printk("%s openwifi_start: DMA channel setup successfully.\n",sdr_compatible_str);

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
			IRQF_SHARED, "sdr,tx_itrpt1", dev);
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

	//ieee80211_wake_queue(dev, 0);

normal_out:
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

	if (test_mode==1){
		pr_info("%s openwifi_stop: test_mode==1\n", sdr_compatible_str);
		goto normal_out;
	}

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

normal_out:
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
	schedule_delayed_work(&vif_priv->beacon_work,
			usecs_to_jiffies(1024 * vif->bss_conf.beacon_int));
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

	printk("%s openwifi_add_interface end with vif idx %d\n", sdr_compatible_str,vif_priv->idx);

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

	if (changed & IEEE80211_CONF_CHANGE_CHANNEL)
		priv->rf->set_chan(dev, conf);
	else
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
			xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16) );
		} else if ((!info->use_short_slot) && priv->use_short_slot==true) {
			priv->use_short_slot=false;
			xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16) );
		}
	}

	if (changed & BSS_CHANGED_BEACON_ENABLED) {
		printk("%s openwifi_bss_info_changed WARNING BSS_CHANGED_BEACON_ENABLED\n",sdr_compatible_str);
		vif_priv->enable_beacon = info->enable_beacon;
	}

	if (changed & (BSS_CHANGED_BEACON_ENABLED | BSS_CHANGED_BEACON)) {
		cancel_delayed_work_sync(&vif_priv->beacon_work);
		if (vif_priv->enable_beacon)
			schedule_work(&vif_priv->beacon_work.work);
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

static int openwifi_conf_tx(struct ieee80211_hw *hw, struct ieee80211_vif *vif, u16 queue,
	      const struct ieee80211_tx_queue_params *params)
{
	u32 reg_val, cw_min_exp, cw_max_exp; 
	
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

	xpu_api->XPU_REG_FILTER_FLAG_write(filter_flag|HIGH_PRIORITY_DISCARD_FLAG);
	//xpu_api->XPU_REG_FILTER_FLAG_write(filter_flag); //do not discard any pkt

	printk("%s openwifi_configure_filter MON %d M_BCN %d BST0 %d BST1 %d UST %d PB_RQ %d PS_PL %d O_BSS %d CTL %d BCN_PRP %d PCP_FL %d FCS_FL %d ALL_MUT %d\n", sdr_compatible_str, 
	(filter_flag>>13)&1,(filter_flag>>12)&1,(filter_flag>>11)&1,(filter_flag>>10)&1,(filter_flag>>9)&1,(filter_flag>>8)&1,(filter_flag>>7)&1,(filter_flag>>6)&1,(filter_flag>>5)&1,(filter_flag>>4)&1,(filter_flag>>3)&1,(filter_flag>>2)&1,(filter_flag>>1)&1);
}

static int openwifi_testmode_cmd(struct ieee80211_hw *hw, struct ieee80211_vif *vif, void *data, int len)
{
	struct openwifi_priv *priv = hw->priv;
	struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
	struct sk_buff *skb;
	int err;
	u32 tmp=-1, reg_cat, reg_addr, reg_val, reg_addr_idx, tsft_high, tsft_low;

	err = nla_parse(tb, OPENWIFI_ATTR_MAX, data, len, openwifi_testmode_policy, NULL);
	if (err)
		return err;

	if (!tb[OPENWIFI_ATTR_CMD])
		return -EINVAL;

	switch (nla_get_u32(tb[OPENWIFI_ATTR_CMD])) {
	case OPENWIFI_CMD_SET_GAP:
		if (!tb[OPENWIFI_ATTR_GAP])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_GAP]);
		printk("%s openwifi radio inter frame gap set to %d usec\n", sdr_compatible_str, tmp);
		xpu_api->XPU_REG_CSMA_CFG_write(tmp); // unit us
		return 0;
	case OPENWIFI_CMD_GET_GAP:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = xpu_api->XPU_REG_CSMA_CFG_read();
		if (nla_put_u32(skb, OPENWIFI_ATTR_GAP, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);
	case OPENWIFI_CMD_SET_SLICE_IDX:
		if (!tb[OPENWIFI_ATTR_SLICE_IDX])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_IDX]);
		printk("%s set openwifi slice_idx in hex: %08x\n", sdr_compatible_str, tmp);
		if (tmp == MAX_NUM_HW_QUEUE) {
			printk("%s set openwifi slice_idx reset all queue counter.\n", sdr_compatible_str);
			xpu_api->XPU_REG_MULTI_RST_write(1<<7); //bit7 reset the counter for all queues at the same time
			xpu_api->XPU_REG_MULTI_RST_write(0<<7); 
		} else {
			priv->slice_idx = tmp;
		}
		return 0;
	case OPENWIFI_CMD_GET_SLICE_IDX:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = priv->slice_idx;
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_IDX, tmp))
			goto nla_put_failure;
		printk("%s get openwifi slice_idx in hex: %08x\n", sdr_compatible_str, tmp);
		return cfg80211_testmode_reply(skb);
	case OPENWIFI_CMD_SET_ADDR:
		if (!tb[OPENWIFI_ATTR_ADDR])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_ADDR]);
		if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
			printk("%s set openwifi slice_target_mac_addr(low32) WARNING: current slice idx %d is invalid!\n", sdr_compatible_str, priv->slice_idx);
		} else {
			printk("%s set openwifi slice_target_mac_addr(low32) in hex: %08x to slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
			priv->dest_mac_addr_queue_map[priv->slice_idx] = reverse32(tmp);
		}
		return 0;
	case OPENWIFI_CMD_GET_ADDR:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
			tmp = -1;
		} else {
			tmp = reverse32(priv->dest_mac_addr_queue_map[priv->slice_idx]);
		}
		if (nla_put_u32(skb, OPENWIFI_ATTR_ADDR, tmp))
			goto nla_put_failure;
		printk("%s get openwifi slice_target_mac_addr(low32) in hex: %08x of slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_TOTAL:
		if (!tb[OPENWIFI_ATTR_SLICE_TOTAL])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_TOTAL]);
		if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
			printk("%s set SLICE_TOTAL(duration) WARNING: current slice idx %d is invalid!\n", sdr_compatible_str, priv->slice_idx);
		} else {
			printk("%s set SLICE_TOTAL(duration) %d usec to slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
			xpu_api->XPU_REG_SLICE_COUNT_TOTAL_write((priv->slice_idx<<20)|tmp);
		}
		return 0;
	case OPENWIFI_CMD_GET_SLICE_TOTAL:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_TOTAL_read());
		printk("%s get SLICE_TOTAL(duration) %d usec of slice %d\n", sdr_compatible_str, tmp&0xFFFFF, tmp>>20);
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_TOTAL, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_START:
		if (!tb[OPENWIFI_ATTR_SLICE_START])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_START]);
		if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
			printk("%s set SLICE_START(duration) WARNING: current slice idx %d is invalid!\n", sdr_compatible_str, priv->slice_idx);
		} else {
			printk("%s set SLICE_START(duration) %d usec to slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
			xpu_api->XPU_REG_SLICE_COUNT_START_write((priv->slice_idx<<20)|tmp);
		}
		return 0;
	case OPENWIFI_CMD_GET_SLICE_START:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_START_read());
		printk("%s get SLICE_START(duration) %d usec of slice %d\n", sdr_compatible_str, tmp&0xFFFFF, tmp>>20);
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_START, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_END:
		if (!tb[OPENWIFI_ATTR_SLICE_END])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_END]);
		if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
			printk("%s set SLICE_END(duration) WARNING: current slice idx %d is invalid!\n", sdr_compatible_str, priv->slice_idx);
		} else {
			printk("%s set SLICE_END(duration) %d usec to slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
			xpu_api->XPU_REG_SLICE_COUNT_END_write((priv->slice_idx<<20)|tmp);
		}
		return 0;
	case OPENWIFI_CMD_GET_SLICE_END:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_END_read());
		printk("%s get SLICE_END(duration) %d usec of slice %d\n", sdr_compatible_str, tmp&0xFFFFF, tmp>>20);
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_END, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	// case OPENWIFI_CMD_SET_SLICE_TOTAL1:
	// 	if (!tb[OPENWIFI_ATTR_SLICE_TOTAL1])
	// 		return -EINVAL;
	// 	tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_TOTAL1]);
	// 	printk("%s set SLICE_TOTAL1(duration) to %d usec\n", sdr_compatible_str, tmp);
	// 	// xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_write(tmp);
	// 	return 0;
	// case OPENWIFI_CMD_GET_SLICE_TOTAL1:
	// 	skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
	// 	if (!skb)
	// 		return -ENOMEM;
	// 	// tmp = (xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_read());
	// 	if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_TOTAL1, tmp))
	// 		goto nla_put_failure;
	// 	return cfg80211_testmode_reply(skb);

	// case OPENWIFI_CMD_SET_SLICE_START1:
	// 	if (!tb[OPENWIFI_ATTR_SLICE_START1])
	// 		return -EINVAL;
	// 	tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_START1]);
	// 	printk("%s set SLICE_START1(duration) to %d usec\n", sdr_compatible_str, tmp);
	// 	// xpu_api->XPU_REG_SLICE_COUNT_START1_write(tmp);
	// 	return 0;
	// case OPENWIFI_CMD_GET_SLICE_START1:
	// 	skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
	// 	if (!skb)
	// 		return -ENOMEM;
	// 	// tmp = (xpu_api->XPU_REG_SLICE_COUNT_START1_read());
	// 	if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_START1, tmp))
	// 		goto nla_put_failure;
	// 	return cfg80211_testmode_reply(skb);

	// case OPENWIFI_CMD_SET_SLICE_END1:
	// 	if (!tb[OPENWIFI_ATTR_SLICE_END1])
	// 		return -EINVAL;
	// 	tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_END1]);
	// 	printk("%s set SLICE_END1(duration) to %d usec\n", sdr_compatible_str, tmp);
	// 	// xpu_api->XPU_REG_SLICE_COUNT_END1_write(tmp);
	// 	return 0;
	// case OPENWIFI_CMD_GET_SLICE_END1:
	// 	skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
	// 	if (!skb)
	// 		return -ENOMEM;
	// 	// tmp = (xpu_api->XPU_REG_SLICE_COUNT_END1_read());
	// 	if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_END1, tmp))
	// 		goto nla_put_failure;
	// 	return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_RSSI_TH:
		if (!tb[OPENWIFI_ATTR_RSSI_TH])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_RSSI_TH]);
		printk("%s set RSSI_TH to %d\n", sdr_compatible_str, tmp);
		xpu_api->XPU_REG_LBT_TH_write(tmp);
		return 0;
	case OPENWIFI_CMD_GET_RSSI_TH:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = xpu_api->XPU_REG_LBT_TH_read();
		if (nla_put_u32(skb, OPENWIFI_ATTR_RSSI_TH, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_TSF:
		printk("openwifi_set_tsf_1");
		if ( (!tb[OPENWIFI_ATTR_HIGH_TSF]) || (!tb[OPENWIFI_ATTR_LOW_TSF]) )
				return -EINVAL;
		printk("openwifi_set_tsf_2");
		tsft_high = nla_get_u32(tb[OPENWIFI_ATTR_HIGH_TSF]);
		tsft_low  = nla_get_u32(tb[OPENWIFI_ATTR_LOW_TSF]);
		xpu_api->XPU_REG_TSF_LOAD_VAL_write(tsft_high,tsft_low);
		printk("%s openwifi_set_tsf: %08x%08x\n", sdr_compatible_str,tsft_high,tsft_low);
		return 0;

	case REG_CMD_SET:
		if ( (!tb[REG_ATTR_ADDR]) || (!tb[REG_ATTR_VAL]) )
			return -EINVAL;
		reg_addr = nla_get_u32(tb[REG_ATTR_ADDR]);
		reg_val  = nla_get_u32(tb[REG_ATTR_VAL]);
		reg_cat = ((reg_addr>>16)&0xFFFF);
		reg_addr = (reg_addr&0xFFFF);
		reg_addr_idx = (reg_addr>>2);
		printk("%s recv set cmd reg cat %d addr %08x val %08x idx %d\n", sdr_compatible_str, reg_cat, reg_addr, reg_val, reg_addr_idx);
		if (reg_cat==1)
			printk("%s reg cat 1 (rf) is not supported yet!\n", sdr_compatible_str);
		else if (reg_cat==2)
			rx_intf_api->reg_write(reg_addr,reg_val);
		else if (reg_cat==3)
			tx_intf_api->reg_write(reg_addr,reg_val);
		else if (reg_cat==4)
			openofdm_rx_api->reg_write(reg_addr,reg_val);
		else if (reg_cat==5)
			openofdm_tx_api->reg_write(reg_addr,reg_val);
		else if (reg_cat==6)
			xpu_api->reg_write(reg_addr,reg_val);
		else if (reg_cat==7) {
			if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
				priv->drv_rx_reg_val[reg_addr_idx]=reg_val;
				if (reg_addr_idx==DRV_RX_REG_IDX_FREQ_BW_CFG) {
					if (reg_val==0)
						priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_0MHZ_ANT0;
					else
						priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_0MHZ_ANT1;

					priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
					//priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];
				}
			} else
				printk("%s reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
		}
		else if (reg_cat==8) {
			if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
				priv->drv_tx_reg_val[reg_addr_idx]=reg_val;
				if (reg_addr_idx==DRV_TX_REG_IDX_FREQ_BW_CFG) {
					if (reg_val==0) {
						priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0;
						ad9361_set_tx_atten(priv->ad9361_phy, AD9361_RADIO_ON_TX_ATT, true, false, true);
					} else {
						priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1;
						ad9361_set_tx_atten(priv->ad9361_phy, AD9361_RADIO_ON_TX_ATT, false, true, true);
					}
					
					//priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
					priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];
				}
			} else
				printk("%s reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
		}
		else if (reg_cat==9) {
			if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG)
				priv->drv_xpu_reg_val[reg_addr_idx]=reg_val;
			else
				printk("%s reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
		}
		else
			printk("%s reg cat %d is not supported yet!\n", sdr_compatible_str, reg_cat);
		
		return 0;
	case REG_CMD_GET:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		reg_addr = nla_get_u32(tb[REG_ATTR_ADDR]);
		reg_cat = ((reg_addr>>16)&0xFFFF);
		reg_addr = (reg_addr&0xFFFF);
		reg_addr_idx = (reg_addr>>2);
		printk("%s recv get cmd reg cat %d addr %08x idx %d\n", sdr_compatible_str, reg_cat, reg_addr, reg_addr_idx);
		if (reg_cat==1) {
			printk("%s reg cat 1 (rf) is not supported yet!\n", sdr_compatible_str);
			tmp = 0xFFFFFFFF;
		}
		else if (reg_cat==2)
			tmp = rx_intf_api->reg_read(reg_addr);
		else if (reg_cat==3)
			tmp = tx_intf_api->reg_read(reg_addr);
		else if (reg_cat==4)
			tmp = openofdm_rx_api->reg_read(reg_addr);
		else if (reg_cat==5)
			tmp = openofdm_tx_api->reg_read(reg_addr);
		else if (reg_cat==6)
			tmp = xpu_api->reg_read(reg_addr);
		else if (reg_cat==7) {
			if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
				if (reg_addr_idx==DRV_RX_REG_IDX_FREQ_BW_CFG) {
					priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
					//priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];

					if (priv->rx_intf_cfg == RX_INTF_BW_20MHZ_AT_0MHZ_ANT0)
						priv->drv_rx_reg_val[reg_addr_idx]=0;
					else if	(priv->rx_intf_cfg == RX_INTF_BW_20MHZ_AT_0MHZ_ANT1)
						priv->drv_rx_reg_val[reg_addr_idx]=1;
				}
				tmp = priv->drv_rx_reg_val[reg_addr_idx];
			} else 
				printk("%s reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
		}
		else if (reg_cat==8) {
			if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
				if (reg_addr_idx==DRV_TX_REG_IDX_FREQ_BW_CFG) {
					//priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
					priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];
					if (priv->tx_intf_cfg == TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0)
						priv->drv_tx_reg_val[reg_addr_idx]=0;
					else if	(priv->tx_intf_cfg == TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1)
						priv->drv_tx_reg_val[reg_addr_idx]=1;
				}
				tmp = priv->drv_tx_reg_val[reg_addr_idx];
			} else
				printk("%s reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
		}
		else if (reg_cat==9) {
			if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG)
				tmp = priv->drv_xpu_reg_val[reg_addr_idx];
			else
				printk("%s reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
		}
		else
			printk("%s reg cat %d is not supported yet!\n", sdr_compatible_str, reg_cat);

		if (nla_put_u32(skb, REG_ATTR_VAL, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	default:
		return -EOPNOTSUPP;
	}

 nla_put_failure:
	dev_kfree_skb(skb);
	return -ENOBUFS;
}

static const struct ieee80211_ops openwifi_ops = {
	.tx			       = openwifi_tx,
	.start			   = openwifi_start,
	.stop			   = openwifi_stop,
	.add_interface	   = openwifi_add_interface,
	.remove_interface  = openwifi_remove_interface,
	.config			   = openwifi_config,
	.bss_info_changed  = openwifi_bss_info_changed,
	.conf_tx		   = openwifi_conf_tx,
	.prepare_multicast = openwifi_prepare_multicast,
	.configure_filter  = openwifi_configure_filter,
	.rfkill_poll	   = openwifi_rfkill_poll,
	.get_tsf		   = openwifi_get_tsf,
	.set_tsf		   = openwifi_set_tsf,
	.reset_tsf		   = openwifi_reset_tsf,
	.set_rts_threshold = openwifi_set_rts_threshold,
	.testmode_cmd	   = openwifi_testmode_cmd,
};

static const struct of_device_id openwifi_dev_of_ids[] = {
	{ .compatible = "sdr,sdr", },
	{}
};
MODULE_DEVICE_TABLE(of, openwifi_dev_of_ids);

static int custom_match_spi_dev(struct device *dev, void *data)
{
    const char *name = data;

	bool ret = sysfs_streq(name, dev->of_node->name);
	printk("%s custom_match_spi_dev %s %s %d\n", sdr_compatible_str,name, dev->of_node->name, ret);
	return ret;
}

static int custom_match_platform_dev(struct device *dev, void *data)
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
	int err=1, rand_val;
	const char *chip_name, *fpga_model;
	u32 reg;//, reg1;

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
		printk("%s openwifi_dev_probe: WARNING unknown openwifi FPGA model %d\n",sdr_compatible_str, err);
		priv->fpga_type = SMALL_FPGA;
	} else {
		// LARGE FPGAs (i.e. ZCU102, Z7035, ZC706)
		if(strstr(fpga_model, "ZCU102") != NULL || strstr(fpga_model, "Z7035") != NULL || strstr(fpga_model, "ZC706") != NULL)
			priv->fpga_type = LARGE_FPGA;
		// SMALL FPGA: (i.e. ZED, ZC702, Z7020)
		else if(strstr(fpga_model, "ZED") != NULL || strstr(fpga_model, "ZC702") != NULL || strstr(fpga_model, "Z7020") != NULL)
			priv->fpga_type = SMALL_FPGA;
	}

	// //-------------find ad9361-phy driver for lo/channel control---------------
	priv->actual_rx_lo = 0;
	tmp_dev = bus_find_device( &spi_bus_type, NULL, "ad9361-phy", custom_match_spi_dev );
	if (tmp_dev == NULL) {
		printk(KERN_ERR "%s find_dev ad9361-phy failed\n",sdr_compatible_str);
		err = -ENOMEM;
		goto err_free_dev;
	}
	printk("%s bus_find_device ad9361-phy: %s. driver_data pointer %p\n", sdr_compatible_str, ((struct spi_device*)tmp_dev)->modalias, (void*)(((struct spi_device*)tmp_dev)->dev.driver_data));
	if (((struct spi_device*)tmp_dev)->dev.driver_data == NULL) {
		printk(KERN_ERR "%s find_dev ad9361-phy failed. dev.driver_data == NULL\n",sdr_compatible_str);
		err = -ENOMEM;
		goto err_free_dev;
	}
	
	priv->ad9361_phy = ad9361_spi_to_phy((struct spi_device*)tmp_dev);
	if (!(priv->ad9361_phy)) {
		printk(KERN_ERR "%s ad9361_spi_to_phy failed\n",sdr_compatible_str);
		err = -ENOMEM;
		goto err_free_dev;
	}
	printk("%s ad9361_spi_to_phy ad9361-phy: %s\n", sdr_compatible_str, priv->ad9361_phy->spi->modalias);

	priv->ctrl_out.en_mask=0xFF;
	priv->ctrl_out.index=0x16;
	err = ad9361_ctrl_outs_setup(priv->ad9361_phy, &(priv->ctrl_out));
	if (err < 0) {
		printk("%s openwifi_dev_probe: WARNING ad9361_ctrl_outs_setup %d\n",sdr_compatible_str, err);
	} else {
		printk("%s openwifi_dev_probe: ad9361_ctrl_outs_setup en_mask 0x%02x index 0x%02x\n",sdr_compatible_str, priv->ctrl_out.en_mask, priv->ctrl_out.index);
	}

	reg = ad9361_spi_read(priv->ad9361_phy->spi, REG_CTRL_OUTPUT_POINTER);
	printk("%s openwifi_dev_probe: ad9361_spi_read REG_CTRL_OUTPUT_POINTER 0x%02x\n",sdr_compatible_str, reg);
	reg = ad9361_spi_read(priv->ad9361_phy->spi, REG_CTRL_OUTPUT_ENABLE);
	printk("%s openwifi_dev_probe: ad9361_spi_read REG_CTRL_OUTPUT_ENABLE 0x%02x\n",sdr_compatible_str, reg);

	// //-------------find driver: axi_ad9361 hdl ref design module, dac channel---------------
	tmp_dev = bus_find_device( &platform_bus_type, NULL, "cf-ad9361-dds-core-lpc", custom_match_platform_dev );
	if (!tmp_dev) {
		printk(KERN_ERR "%s bus_find_device platform_bus_type cf-ad9361-dds-core-lpc failed\n",sdr_compatible_str);
		err = -ENOMEM;
		goto err_free_dev;
	}

	tmp_pdev = to_platform_device(tmp_dev);
	if (!tmp_pdev) {
		printk(KERN_ERR "%s to_platform_device failed\n",sdr_compatible_str);
		err = -ENOMEM;
		goto err_free_dev;
	}

	tmp_indio_dev = platform_get_drvdata(tmp_pdev);
	if (!tmp_indio_dev) {
		printk(KERN_ERR "%s platform_get_drvdata failed\n",sdr_compatible_str);
		err = -ENOMEM;
		goto err_free_dev;
	}

	priv->dds_st = iio_priv(tmp_indio_dev);
	if (!(priv->dds_st)) {
		printk(KERN_ERR "%s iio_priv failed\n",sdr_compatible_str);
		err = -ENOMEM;
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
	// 	priv->rfkill_off = 0;// 0 off, 1 on
	// 	printk("%s openwifi_dev_probe: rfkill radio off\n",sdr_compatible_str);
	// }
	// else
	// 	printk("%s openwifi_dev_probe: WARNING rfkill radio off failed. tx att read %d %d require %d\n",sdr_compatible_str, reg, reg1, AD9361_RADIO_OFF_TX_ATT);
	
	priv->rssi_correction = 43;//this will be set in real-time by _rf_set_channel()

	//priv->rf_bw = 20000000; // Signal quality issue! NOT use for now. 20MHz or 40MHz. 40MHz need ddc/duc. 20MHz works in bypass mode
	priv->rf_bw = 40000000; // 20MHz or 40MHz. 40MHz need ddc/duc. 20MHz works in bypass mode

	priv->xpu_cfg = XPU_NORMAL;

	priv->openofdm_tx_cfg = OPENOFDM_TX_NORMAL;
	priv->openofdm_rx_cfg = OPENOFDM_RX_NORMAL;

	printk("%s openwifi_dev_probe: priv->rf_bw == %dHz. bool for 20000000 %d, 40000000 %d\n",sdr_compatible_str, priv->rf_bw, (priv->rf_bw==20000000) , (priv->rf_bw==40000000) );
	if (priv->rf_bw == 20000000) {
		priv->rx_intf_cfg = RX_INTF_BYPASS;
		priv->tx_intf_cfg = TX_INTF_BYPASS;
		//priv->rx_freq_offset_to_lo_MHz = 0;
		//priv->tx_freq_offset_to_lo_MHz = 0;
	} else if (priv->rf_bw == 40000000) {
		//priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_P_10MHZ; //work
		//priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1; //work

		// // test ddc at central, duc at central+10M. It works. And also change rx BW from 40MHz to 20MHz in rf_init.sh. Rx sampling rate is still 40Msps
		priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_0MHZ_ANT0;
		priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0; // Let's use rx0 tx0 as default mode, because it works for both 9361 and 9364
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
	}
	priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
	priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];
	printk("%s openwifi_dev_probe: test_mode %d\n", sdr_compatible_str, test_mode);

	//let's by default turn radio on when probing
	if (priv->tx_intf_cfg == TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1) {
		ad9361_set_tx_atten(priv->ad9361_phy, AD9361_RADIO_ON_TX_ATT, false, true, true); // AD9361_RADIO_ON_TX_ATT 3000 means 3dB, 0 means 0dB
		reg = ad9361_get_tx_atten(priv->ad9361_phy, 2);
	} else {
		ad9361_set_tx_atten(priv->ad9361_phy, AD9361_RADIO_ON_TX_ATT, true, false, true); // AD9361_RADIO_ON_TX_ATT 3000 means 3dB, 0 means 0dB
		reg = ad9361_get_tx_atten(priv->ad9361_phy, 1);
	}
	if (reg == AD9361_RADIO_ON_TX_ATT) {
		priv->rfkill_off = 1;// 0 off, 1 on
		printk("%s openwifi_dev_probe: rfkill radio on\n",sdr_compatible_str);
	}
	else
		printk("%s openwifi_dev_probe: WARNING rfkill radio on failed. tx att read %d require %d\n",sdr_compatible_str, reg, AD9361_RADIO_ON_TX_ATT);
	
	memset(priv->drv_rx_reg_val,0,sizeof(priv->drv_rx_reg_val));
	memset(priv->drv_tx_reg_val,0,sizeof(priv->drv_tx_reg_val));
	memset(priv->drv_xpu_reg_val,0,sizeof(priv->drv_xpu_reg_val));

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

	priv->band = BAND_5_8GHZ; //this can be changed by band _rf_set_channel() (2.4GHz ERP(OFDM)) (5GHz OFDM)
	priv->channel = 44;  //currently useless. this can be changed by band _rf_set_channel()
	priv->use_short_slot = false; //this can be changed by openwifi_bss_info_changed: BSS_CHANGED_ERP_SLOT

	priv->band_2GHz.band = NL80211_BAND_2GHZ;
	priv->band_2GHz.channels = priv->channels_2GHz;
	priv->band_2GHz.n_channels = ARRAY_SIZE(priv->channels_2GHz);
	priv->band_2GHz.bitrates = priv->rates_2GHz;
	priv->band_2GHz.n_bitrates = ARRAY_SIZE(priv->rates_2GHz);
	priv->band_2GHz.ht_cap.ht_supported = true;
	priv->band_2GHz.ht_cap.cap = IEEE80211_HT_CAP_SGI_20;
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
	priv->band_5GHz.ht_cap.cap = IEEE80211_HT_CAP_SGI_20;
	memset(&priv->band_5GHz.ht_cap.mcs, 0, sizeof(priv->band_5GHz.ht_cap.mcs));
	priv->band_5GHz.ht_cap.mcs.rx_mask[0] = 0xff;
	priv->band_5GHz.ht_cap.mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;
	dev->wiphy->bands[NL80211_BAND_5GHZ] = &(priv->band_5GHz);

	printk("%s openwifi_dev_probe: band_2GHz.n_channels %d n_bitrates %d band_5GHz.n_channels %d n_bitrates %d\n",sdr_compatible_str,
	priv->band_2GHz.n_channels,priv->band_2GHz.n_bitrates,priv->band_5GHz.n_channels,priv->band_5GHz.n_bitrates);

	ieee80211_hw_set(dev, HOST_BROADCAST_PS_BUFFERING);
	ieee80211_hw_set(dev, RX_INCLUDES_FCS);
	ieee80211_hw_set(dev, BEACON_TX_STATUS);

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

	dev->wiphy->regulatory_flags = (REGULATORY_STRICT_REG|REGULATORY_CUSTOM_REG); // use our own config within strict regulation
	//dev->wiphy->regulatory_flags = REGULATORY_CUSTOM_REG; // use our own config
	wiphy_apply_custom_regulatory(dev->wiphy, &sdr_regd);

	chip_name = "ZYNQ";

	/* we declare to MAC80211 all the queues except for beacon queue
	 * that will be eventually handled by DRV.
	 * TX rings are arranged in such a way that lower is the IDX,
	 * higher is the priority, in order to achieve direct mapping
	 * with mac80211, however the beacon queue is an exception and it
	 * is mapped on the highst tx ring IDX.
	 */
	dev->queues = MAX_NUM_HW_QUEUE;
	//dev->queues = 1;

	ieee80211_hw_set(dev, SIGNAL_DBM);

	wiphy_ext_feature_set(dev->wiphy, NL80211_EXT_FEATURE_CQM_RSSI_LIST);

	priv->rf = &ad9361_rf_ops;

	memset(priv->dest_mac_addr_queue_map,0,sizeof(priv->dest_mac_addr_queue_map));
	priv->slice_idx = 0xFFFFFFFF;

	sg_init_table(&(priv->tx_sg), 1);

	get_random_bytes(&rand_val, sizeof(rand_val));
    rand_val%=250;
	priv->mac_addr[0]=0x66;	priv->mac_addr[1]=0x55;	priv->mac_addr[2]=0x44;	priv->mac_addr[3]=0x33;	priv->mac_addr[4]=0x22;
	priv->mac_addr[5]=rand_val+1;
	//priv->mac_addr[5]=0x11;
	if (!is_valid_ether_addr(priv->mac_addr)) {
		printk(KERN_WARNING "%s openwifi_dev_probe: WARNING Invalid hwaddr! Using randomly generated MAC addr\n",sdr_compatible_str);
		eth_random_addr(priv->mac_addr);
	} else {
		printk("%s openwifi_dev_probe: mac_addr %02x:%02x:%02x:%02x:%02x:%02x\n",sdr_compatible_str,priv->mac_addr[0],priv->mac_addr[1],priv->mac_addr[2],priv->mac_addr[3],priv->mac_addr[4],priv->mac_addr[5]);
	}
	SET_IEEE80211_PERM_ADDR(dev, priv->mac_addr);

	spin_lock_init(&priv->lock);

	err = ieee80211_register_hw(dev);
	if (err) {
		pr_err(KERN_ERR "%s openwifi_dev_probe: WARNING Cannot register device\n",sdr_compatible_str);
		goto err_free_dev;
	} else {
		printk("%s openwifi_dev_probe: ieee80211_register_hw %d\n",sdr_compatible_str, err);
	}

	// // //--------------------hook leds (not complete yet)--------------------------------
	// tmp_dev = bus_find_device( &platform_bus_type, NULL, "leds", custom_match_platform_dev ); //leds is the name in devicetree, not "compatible" field
	// if (!tmp_dev) {
	// 	printk(KERN_ERR "%s bus_find_device platform_bus_type leds-gpio failed\n",sdr_compatible_str);
	// 	err = -ENOMEM;
	// 	goto err_free_dev;
	// }

	// tmp_pdev = to_platform_device(tmp_dev);
	// if (!tmp_pdev) {
	// 	printk(KERN_ERR "%s to_platform_device failed for leds-gpio\n",sdr_compatible_str);
	// 	err = -ENOMEM;
	// 	goto err_free_dev;
	// }

	// tmp_led_priv = platform_get_drvdata(tmp_pdev);
	// if (!tmp_led_priv) {
	// 	printk(KERN_ERR "%s platform_get_drvdata failed for leds-gpio\n",sdr_compatible_str);
	// 	err = -ENOMEM;
	// 	goto err_free_dev;
	// }
	// printk("%s openwifi_dev_probe: leds-gpio detect %d leds!\n",sdr_compatible_str, tmp_led_priv->num_leds);
	// if (tmp_led_priv->num_leds!=4){
	// 	printk(KERN_ERR "%s WARNING we expect 4 leds, but actual %d leds\n",sdr_compatible_str,tmp_led_priv->num_leds);
	// 	err = -ENOMEM;
	// 	goto err_free_dev;
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
	
	wiphy_info(dev->wiphy, "hwaddr %pm, %s + %s\n",
		   priv->mac_addr, chip_name, priv->rf->name);

	openwifi_rfkill_init(dev);
	return 0;

 err_free_dev:
	ieee80211_free_hw(dev);

	return err;
}

static int openwifi_dev_remove(struct platform_device *pdev)
{
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);

	if (!dev) {
		pr_info("%s openwifi_dev_remove: dev %p\n", sdr_compatible_str, (void*)dev);
		return(-1);
	}

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
