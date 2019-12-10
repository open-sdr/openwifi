//Author: Xianjun Jiao. putaoshu@msn.com; xianjun.jiao@imec.be

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
#include "ad9361/ad9361_regs.h"
#include "ad9361/ad9361.h"
#include "ad9361/ad9361_private.h"

#include <../../drivers/iio/frequency/cf_axi_dds.h>

#include "../user_space/sdrctl_src/nl80211_testmode_def.h"
#include "hw_def.h"
#include "sdr.h"

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
	printk("%s openwifi_rfkill_poll: wireless radio switch turned %s\n", sdr_compatible_str, enabled ? "on" : "off");
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
	u32 actual_rx_lo = conf->chandef.chan->center_freq - priv->rx_freq_offset_to_lo_MHz;
	u32 actual_tx_lo;
	bool change_flag = (actual_rx_lo != priv->actual_rx_lo);

	if (change_flag) {
		priv->actual_rx_lo = actual_rx_lo;

		actual_tx_lo = conf->chandef.chan->center_freq - priv->tx_freq_offset_to_lo_MHz;

		ad9361_clk_set_rate(priv->ad9361_phy->clks[RX_RFPLL], ( ((u64)1000000ull)*((u64)actual_rx_lo )>>1) );
		ad9361_clk_set_rate(priv->ad9361_phy->clks[TX_RFPLL], ( ((u64)1000000ull)*((u64)actual_tx_lo )>>1) );

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
		xpu_api->XPU_REG_LBT_TH_write((priv->rssi_correction-62)<<1);

		if (actual_rx_lo < 2500) {
			//priv->slot_time = 20; //20 is default slot time in ERP(OFDM)/11g 2.4G; short one is 9.
			//xpu_api->XPU_REG_BAND_CHANNEL_write(BAND_2_4GHZ<<16);
			if (priv->band != BAND_2_4GHZ) {
				priv->band = BAND_2_4GHZ;
				xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16) );
			}
			// //xpu_api->XPU_REG_RECV_ACK_COUNT_TOP_write( (((45+2)*200)<<16) | 200 ); // high 16 bits to cover sig valid of ACK packet, low 16 bits is adjustment of fcs valid waiting time.  let's add 2us for those device that is really "slow"!
			// xpu_api->XPU_REG_RECV_ACK_COUNT_TOP_write( (((45+2+2)*200)<<16) | 200 );//add 2us for longer fir. BUT corrding to FPGA probing test, we do not need this
			// xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( 0 );
			// tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write(((10)*200)<<16);
		}
		else {
			//priv->slot_time = 9; //default slot time of OFDM PHY (OFDM by default means 5GHz)
			// xpu_api->XPU_REG_BAND_CHANNEL_write(BAND_5_8GHZ<<16);
			if (priv->band != BAND_5_8GHZ) {
				priv->band = BAND_5_8GHZ;
				xpu_api->XPU_REG_BAND_CHANNEL_write( (priv->use_short_slot<<24)|(priv->band<<16) );
			}
			// //xpu_api->XPU_REG_RECV_ACK_COUNT_TOP_write( (((51+2)*200)<<16) | 200 ); // because 5GHz needs longer SIFS (16 instead of 10), we need 58 instead of 48 for XPU low mac setting.  let's add 2us for those device that is really "slow"!
			// xpu_api->XPU_REG_RECV_ACK_COUNT_TOP_write( (((51+2+2)*200)<<16) | 200 );//add 2us for longer fir.  BUT corrding to FPGA probing test, we do not need this
			// //xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( 1200 );
			// xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( 1000 );// for longer fir we need this delay 1us shorter
			// tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write(((16)*200)<<16);
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

static int openwifi_init_tx_ring(struct openwifi_priv *priv)
{
	struct openwifi_ring *ring = &(priv->tx_ring);
	int i;

	priv->tx_queue_stopped = false;
	ring->bd_wr_idx = 0;
	ring->bd_rd_idx = 0;
	ring->bds = kmalloc(sizeof(struct openwifi_buffer_descriptor)*NUM_TX_BD,GFP_KERNEL);
	if (ring->bds==NULL) {
		printk("%s openwifi_init_tx_ring: WARNING Cannot allocate TX ring\n",sdr_compatible_str);
		return -ENOMEM;
	}

	for (i = 0; i < NUM_TX_BD; i++) {
		ring->bds[i].num_dma_byte=0;
		ring->bds[i].sn=0;
		ring->bds[i].hw_queue_idx=0;
		ring->bds[i].retry_limit=0;
		ring->bds[i].need_ack=0;

		ring->bds[i].skb_linked=0; // for tx, skb is from upper layer
		//at frist right after skb allocated, head, data, tail are the same.
		ring->bds[i].dma_mapping_addr = 0; // for tx, mapping is done after skb is received from uppler layer in tx routine
	}

	return 0;
}

static void openwifi_free_tx_ring(struct openwifi_priv *priv)
{
	struct openwifi_ring *ring = &(priv->tx_ring);
	int i;

	ring->bd_wr_idx = 0;
	ring->bd_rd_idx = 0;
	for (i = 0; i < NUM_TX_BD; i++) {
		ring->bds[i].num_dma_byte=0;
		ring->bds[i].sn=0;
		ring->bds[i].hw_queue_idx=0;
		ring->bds[i].retry_limit=0;
		ring->bds[i].need_ack=0;

		if (ring->bds[i].skb_linked == 0 && ring->bds[i].dma_mapping_addr == 0)
			continue;
		if (ring->bds[i].dma_mapping_addr != 0)
			dma_unmap_single(priv->tx_chan->device->dev, ring->bds[i].dma_mapping_addr,ring->bds[i].num_dma_byte, DMA_MEM_TO_DEV);
//		if (ring->bds[i].skb_linked!=NULL)
//			dev_kfree_skb(ring->bds[i].skb_linked);
		if ( (ring->bds[i].dma_mapping_addr != 0 && ring->bds[i].skb_linked == 0) ||
		     (ring->bds[i].dma_mapping_addr == 0 && ring->bds[i].skb_linked != 0))
			printk("%s openwifi_free_tx_ring: WARNING %d skb_linked %08x dma_mapping_addr %08x\n", sdr_compatible_str, i, (u32)(ring->bds[i].skb_linked), ring->bds[i].dma_mapping_addr);

		ring->bds[i].skb_linked=0;
		ring->bds[i].dma_mapping_addr = 0;
	}
	if (ring->bds)
		kfree(ring->bds);
	ring->bds = NULL;
}

static int openwifi_init_rx_ring(struct openwifi_priv *priv)
{
	priv->rx_cyclic_buf = dma_alloc_coherent(priv->rx_chan->device->dev,RX_BD_BUF_SIZE*NUM_RX_BD,&priv->rx_cyclic_buf_dma_mapping_addr,GFP_KERNEL);
	if (!priv->rx_cyclic_buf) {
		printk("%s openwifi_init_rx_ring: WARNING dma_alloc_coherent failed!\n", sdr_compatible_str);
		dma_free_coherent(priv->rx_chan->device->dev,RX_BD_BUF_SIZE*NUM_RX_BD,priv->rx_cyclic_buf,priv->rx_cyclic_buf_dma_mapping_addr);
		return(-1);
	}
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
		printk("%s rx_dma_setup: WARNING rx_dev->device_prep_dma_cyclic %d\n", sdr_compatible_str, (u32)(priv->rxd));
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
	u32 addr1_low32=0, addr2_low32=0, addr3_low32=0, len, rate_idx, ht_flag, tsft_low, tsft_high;//, fc_di;
	u32 dma_driver_buf_idx_mod;
	u8 *pdata_tmp, fcs_ok, phy_rx_sn_hw, target_buf_idx;
	s8 signal;
	u16 rssi_val, addr1_high16=0, addr2_high16=0, addr3_high16=0, sc=0;
	bool content_ok = false;
	struct dma_tx_state state;
	static u8 target_buf_idx_old = 0xFF;

	spin_lock(&priv->lock);
	priv->rx_chan->device->device_tx_status(priv->rx_chan,priv->rx_cookie,&state);
	target_buf_idx = ((state.residue-1)&(NUM_RX_BD-1));
	if (target_buf_idx==target_buf_idx_old) {
		//printk("%s openwifi_rx_interrupt: WARNING same idx %d\n", sdr_compatible_str,target_buf_idx);
		goto openwifi_rx_interrupt_out;
	}
	if ( ((target_buf_idx-target_buf_idx_old)&(NUM_RX_BD-1))!=1 )
		printk("%s openwifi_rx_interrupt: WARNING jump idx target %d old %d diff %02x\n", sdr_compatible_str,target_buf_idx,target_buf_idx_old,((target_buf_idx-target_buf_idx_old)&(NUM_RX_BD-1)));
	target_buf_idx_old = target_buf_idx;

	pdata_tmp = priv->rx_cyclic_buf + target_buf_idx*RX_BD_BUF_SIZE; // our header insertion is at the beginning
	tsft_low =     (*((u32*)(pdata_tmp+0 )));
	tsft_high =    (*((u32*)(pdata_tmp+4 )));
	rssi_val =     (*((u16*)(pdata_tmp+8 )));
	len =          (*((u16*)(pdata_tmp+12)));
	//len_new = (((len>>3) + ((len&0x7)!=0))<<3);
	rate_idx =     (*((u16*)(pdata_tmp+14)));

	// fc_di =        (*((u32*)(pdata_tmp+16)));
	// addr1_high16 = (*((u16*)(pdata_tmp+16+4)));
	// addr1_low32  = (*((u32*)(pdata_tmp+16+4+2)));
	// addr2_high16 = (*((u16*)(pdata_tmp+16+6+4)));
	// addr2_low32  = (*((u32*)(pdata_tmp+16+6+4+2)));
	// addr3_high16 = (*((u16*)(pdata_tmp+16+12+4)));
	// addr3_low32  = (*((u32*)(pdata_tmp+16+12+4+2)));
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

	fcs_ok =       (*(( u8*)(pdata_tmp+16+len-1)));

	phy_rx_sn_hw = (fcs_ok&0x7f);//0x7f is FPGA limitation
	dma_driver_buf_idx_mod = (state.residue&0x7f);
	//phy_rx_sn_hw = (fcs_ok&(NUM_RX_BD-1));
	fcs_ok = ((fcs_ok&0x80)!=0);
	ht_flag = ((rate_idx&0x10)!=0);
	rate_idx = (rate_idx&0xF);

	if ( (len>=14 && len<=8191) && (rate_idx>=8 && rate_idx<=15)) {
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
	if (addr1_low32!=0xffffffff && addr1_high16!=0xffff)
	printk("%s openwifi_rx_interrupt:%4dbytes ht%d %2dM FC%04x DI%04x addr1/2/3:%04x%08x/%04x%08x/%04x%08x SC%04x fcs%d sn%d i%d %ddBm\n", sdr_compatible_str,
	len, ht_flag, wifi_rate_table[rate_idx], hdr->frame_control,hdr->duration_id, 
	reverse16(addr1_high16), reverse32(addr1_low32), reverse16(addr2_high16), reverse32(addr2_low32), reverse16(addr3_high16), reverse32(addr3_low32), 
	sc,fcs_ok, phy_rx_sn_hw,dma_driver_buf_idx_mod,signal);

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
			rx_status.encoding = RX_ENC_LEGACY;
			rx_status.bw = RATE_INFO_BW_20;

			memcpy(IEEE80211_SKB_RXCB(skb), &rx_status, sizeof(rx_status)); // put rx_status into skb->cb, from now on skb->cb is not dma_dsts any more.
			ieee80211_rx_irqsafe(dev, skb); // call mac80211 function
		} else
			printk("%s openwifi_rx_interrupt: WARNING skb!\n", sdr_compatible_str);
	}
openwifi_rx_interrupt_out:
	spin_unlock(&priv->lock);
	return IRQ_HANDLED;
}

static irqreturn_t openwifi_tx_interrupt(int irq, void *dev_id)
{
	struct ieee80211_hw *dev = dev_id;
	struct openwifi_priv *priv = dev->priv;
	struct openwifi_ring *ring = &(priv->tx_ring);
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;
	u32 reg_val,ring_len, ring_room_left, just_wr_idx, current_rd_idx; //queue_idx_hw, ;
	u32 num_dma_byte_hw;
	u32 phy_tx_sn_hw;
	u8 tx_result;

	spin_lock(&priv->lock);

	tx_result = xpu_api->XPU_REG_TX_RESULT_read();
	reg_val = tx_intf_api->TX_INTF_REG_PKT_INFO_read();// current interrupt is the end of phy_tx_sn_hw pkt transmitting.
	num_dma_byte_hw = (reg_val&0xFFFF);
	phy_tx_sn_hw = ((reg_val>>16)&MAX_PHY_TX_SN);
	//queue_idx_hw = (reg_val&(MAX_NUM_HW_QUEUE-1));

	//just_wr_idx = (ring->bd_wr_idx==0?(NUM_TX_BD-1):(ring->bd_wr_idx-1));
	just_wr_idx = ((ring->bd_wr_idx-1)&(NUM_TX_BD-1));
	while(1) {
		current_rd_idx = ring->bd_rd_idx;

		dma_unmap_single(priv->tx_chan->device->dev,ring->bds[current_rd_idx].dma_mapping_addr,
				ring->bds[current_rd_idx].num_dma_byte, DMA_MEM_TO_DEV);

		if (phy_tx_sn_hw != ring->bds[current_rd_idx].sn) {
			ring->bd_rd_idx = ((ring->bd_rd_idx+1)&(NUM_TX_BD-1));
			if (current_rd_idx == just_wr_idx) {
				printk("%s openwifi_tx_interrupt: WARNING can not find hw sn %d in driver! curr rd %d just wr %d\n", sdr_compatible_str,phy_tx_sn_hw,current_rd_idx,just_wr_idx);
				break;
			} else
				continue;
		}

		// a know bd has just been sent to the air
		if (num_dma_byte_hw!=ring->bds[current_rd_idx].num_dma_byte) {
			ring->bd_rd_idx = ((ring->bd_rd_idx+1)&(NUM_TX_BD-1));
			printk("%s openwifi_tx_interrupt: WARNING num_dma_byte is different %d VS %d at sn %d curr rd %d just wr %d\n", sdr_compatible_str,num_dma_byte_hw,ring->bds[current_rd_idx].num_dma_byte,phy_tx_sn_hw,current_rd_idx,just_wr_idx);
			if (current_rd_idx == just_wr_idx)
				break;
			else 
				continue;
		}

		// num_dma_byte_hw is correct
		skb = ring->bds[current_rd_idx].skb_linked;
		// dma_buf = skb->data;
		//phy_tx_sn_skb = (*((u16*)(dma_buf+6)));
		//num_dma_byte_skb = (*((u32*)(dma_buf+8)));
		//num_byte_pad_skb = (*((u32*)(dma_buf+12)));
		
		//if ( phy_tx_sn_hw!=phy_tx_sn_entry || phy_tx_sn_hw!=phy_tx_sn_skb || phy_tx_sn_entry!=phy_tx_sn_skb )
		//	printk("%s openwifi_tx_interrupt: WARNING hw/entry/skb num byte %d/%d/%d pkt sn %d/%d/%d pad %d\n", sdr_compatible_str, 
		//	num_dma_byte_hw, num_dma_byte_entry, num_dma_byte_skb, phy_tx_sn_hw, phy_tx_sn_entry, phy_tx_sn_skb, num_byte_pad_skb);

		skb_pull(skb, LEN_PHY_HEADER);
		//skb_trim(skb, num_byte_pad_skb);
		info = IEEE80211_SKB_CB(skb);
		ieee80211_tx_info_clear_status(info);

		if ( !(info->flags & IEEE80211_TX_CTL_NO_ACK) ) {
			if ((tx_result&0x10)==0)
				info->flags |= IEEE80211_TX_STAT_ACK;

			// printk("%s openwifi_tx_interrupt: rate&try: %d %d %03x; %d %d %03x; %d %d %03x; %d %d %03x\n", sdr_compatible_str,
			// 	info->status.rates[0].idx,info->status.rates[0].count,info->status.rates[0].flags,
			// 	info->status.rates[1].idx,info->status.rates[1].count,info->status.rates[1].flags,
			// 	info->status.rates[2].idx,info->status.rates[2].count,info->status.rates[2].flags,
			// 	info->status.rates[3].idx,info->status.rates[3].count,info->status.rates[3].flags);
		}

		info->status.rates[0].count = (tx_result&0xF) + 1; //according to our test, the 1st rate is the most important. we only do retry on the 1st rate
		info->status.rates[1].idx = -1;
		info->status.rates[2].idx = -1;
		info->status.rates[3].idx = -1;//in mac80211.h: #define IEEE80211_TX_MAX_RATES	4
		if (tx_result&0x10)
			printk("%s openwifi_tx_interrupt: WARNING tx_result %02x phy_tx_sn_hw %d. curr rd %d just wr %d\n", sdr_compatible_str,tx_result,phy_tx_sn_hw,current_rd_idx,just_wr_idx);

		ieee80211_tx_status_irqsafe(dev, skb);
		//ring_len = (just_wr_idx>=current_rd_idx)?(just_wr_idx-current_rd_idx):(just_wr_idx+NUM_TX_BD-current_rd_idx);
		ring_len = ((just_wr_idx-current_rd_idx)&(NUM_TX_BD-1));
		ring_room_left = NUM_TX_BD - ring_len;
		if (ring_room_left > 2 && priv->tx_queue_stopped) {
			unsigned int prio = skb_get_queue_mapping(skb);
			ieee80211_wake_queue(dev, prio);
			printk("%s openwifi_tx_interrupt: WARNING ieee80211_wake_queue. ring_room_left %d prio %d curr rd %d just wr %d\n", sdr_compatible_str,ring_room_left,prio,current_rd_idx,just_wr_idx);
			priv->tx_queue_stopped = false;
		}

		ring->bd_rd_idx = ((ring->bd_rd_idx+1)&(NUM_TX_BD-1));

		//if (current_rd_idx == just_wr_idx)
			break; // we have hit the sn, we should break
	}

	spin_unlock(&priv->lock);

	return IRQ_HANDLED;
}

u32 gen_parity(u32 v){
	v ^= v >> 1;
	v ^= v >> 2;
	v = (v & 0x11111111U) * 0x11111111U;
	return (v >> 28) & 1;
}

u32 calc_phy_header(u8 rate_hw_value, u32 len, u8 *bytes){
	//u32 signal_word = 0 ;
	u8  SIG_RATE = 0 ;
	u8	len_2to0, len_10to3, len_msb,b0,b1,b2, header_parity ;

	// rate_hw_value = (rate_hw_value<=4?0:(rate_hw_value-4));
	// SIG_RATE = wifi_mcs_table_phy_tx[rate_hw_value];
	SIG_RATE = wifi_mcs_table_11b_force_up[rate_hw_value];

	len_2to0 = len & 0x07 ;
	len_10to3 = (len >> 3 ) & 0xFF ;
	len_msb = (len >> 11) & 0x01 ;

	b0=SIG_RATE | (len_2to0 << 5) ;
	b1 = len_10to3 ;
	header_parity = gen_parity((len_msb << 16)| (b1<<8) | b0) ;
	b2 = ( len_msb | (header_parity << 1) ) ;

	memset(bytes,0,16);
	bytes[0] = b0 ;
	bytes[1] = b1 ; 
    bytes[2] = b2;
	//signal_word = b0+(b1<<8)+(b2<<16) ;
	//return signal_word;
	return(SIG_RATE);
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
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
	struct openwifi_priv *priv = dev->priv;
	struct openwifi_ring *ring = &(priv->tx_ring);
	dma_addr_t dma_mapping_addr;
	unsigned long flags;
	unsigned int prio, i;
	u32 num_dma_symbol, len_mac_pdu, num_dma_byte, len_phy_packet, num_byte_pad;
	u32 rate_signal_value,rate_hw_value,ack_flag;
	u32 pkt_need_ack, addr1_low32=0, addr2_low32=0, addr3_low32=0, queue_idx=2, ring_len, ring_room_left, dma_reg, cts_reg;//, openofdm_state_history;
	u16 addr1_high16=0, addr2_high16=0, addr3_high16=0, sc=0, cts_duration=0, cts_rate_hw_value = 0, cts_rate_signal_value=0, sifs, ack_duration=0, traffic_pkt_duration;
	u8 fc_flag,fc_type,fc_subtype,retry_limit_raw,*dma_buf,retry_limit_hw_value,rc_flags;
	bool use_rts_cts, use_cts_protect, force_use_cts_protect=false, addr_flag, cts_use_traffic_rate;
	__le16 frame_control,duration_id;
	// static u32 openofdm_state_history_old=0;
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
		printk("%s openwifi_tx: test_mode==1\n", sdr_compatible_str);
		goto openwifi_tx_early_out;
	}
	if (skb->data_len>0)// more data are not in linear data area skb->data
		goto openwifi_tx_early_out;

	len_mac_pdu = skb->len;
	len_phy_packet = len_mac_pdu + LEN_PHY_HEADER;
	num_dma_symbol = (len_phy_packet>>TX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS) + ((len_phy_packet&(TX_INTF_NUM_BYTE_PER_DMA_SYMBOL-1))!=0);
	num_dma_byte = (num_dma_symbol<<TX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS);
	if (num_dma_byte > TX_BD_BUF_SIZE) {
		dev_err(priv->tx_chan->device->dev, "WARNING num_dma_byte > TX_BD_BUF_SIZE\n");
		goto openwifi_tx_early_out;
	}
	num_byte_pad = num_dma_byte-len_phy_packet;

	// -----------preprocess some info from header and skb----------------
	prio = skb_get_queue_mapping(skb);
	if (prio) {
		printk("%s openwifi_tx: WARNING prio %d\n", sdr_compatible_str, prio);
	}

	rate_hw_value = ieee80211_get_tx_rate(dev, info)->hw_value;

	addr1_low32  = *((u32*)(hdr->addr1+2));
	addr1_high16 = *((u16*)(hdr->addr1));
	if (len_mac_pdu>=20) {
		addr2_low32  = *((u32*)(hdr->addr2+2));
		addr2_high16 = *((u16*)(hdr->addr2));
	}
	if (len_mac_pdu>=26) {
		addr3_low32  = *((u32*)(hdr->addr3+2));
		addr3_high16 = *((u16*)(hdr->addr3));
	}
	if (len_mac_pdu>=28)
		sc = hdr->seq_ctrl;

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

	//rate_hw_value = 10; //4:6M, 5:9M, 6:12M, 7:18M, 8:24M, 9:36M, 10:48M, 11:54M
	if (priv->drv_tx_reg_val[0]>0 && fc_type==2 && (!addr_flag))
		rate_hw_value = priv->drv_tx_reg_val[0];
		
	// check current packet belonging to which slice/hw-queue
	for (i=0; i<MAX_NUM_HW_QUEUE; i++) {
		if ( priv->dest_mac_addr_queue_map[i] == addr1_low32 && ( !addr_flag ) ) {
			break;
		}
	}
	queue_idx = i;
	if (i>=MAX_NUM_HW_QUEUE)
		queue_idx = 0;

	retry_limit_raw = info->control.rates[0].count;

	rc_flags = info->control.rates[0].flags;
	use_rts_cts = ((rc_flags&IEEE80211_TX_RC_USE_RTS_CTS)!=0);
	use_cts_protect = ((rc_flags&IEEE80211_TX_RC_USE_CTS_PROTECT)!=0);

	if (use_rts_cts)
		printk("%s openwifi_tx: WARNING use_rts_cts is not supported!\n", sdr_compatible_str);

	cts_use_traffic_rate = false;
	force_use_cts_protect = false;
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

	//if (addr1_low32!=0xffffffff && addr1_high16!=0xffff)
	if ( !addr_flag ) {
		printk("%s openwifi_tx: %4dbytes %2dM FC%04x DI%04x addr1/2/3:%04x%08x/%04x%08x/%04x%08x SC%04x flag%08x retry%d ack%d q%d sn%04d R/CTS %d%d %dM %dus wr/rd %d/%d\n", sdr_compatible_str,
			len_mac_pdu, wifi_rate_all[rate_hw_value],frame_control,duration_id, 
			reverse16(addr1_high16), reverse32(addr1_low32), reverse16(addr2_high16), reverse32(addr2_low32), reverse16(addr3_high16), reverse32(addr3_low32),
			sc,info->flags,retry_limit_raw,pkt_need_ack,queue_idx,priv->phy_tx_sn,
			use_rts_cts,use_cts_protect|force_use_cts_protect,wifi_rate_all[cts_rate_hw_value],cts_duration,
			ring->bd_wr_idx,ring->bd_rd_idx);
		// printk("%s openwifi_tx: rate&try: %d %d %03x; %d %d %03x; %d %d %03x; %d %d %03x\n", sdr_compatible_str,
		// 	info->status.rates[0].idx,info->status.rates[0].count,info->status.rates[0].flags,
		// 	info->status.rates[1].idx,info->status.rates[1].count,info->status.rates[1].flags,
		// 	info->status.rates[2].idx,info->status.rates[2].count,info->status.rates[2].flags,
		// 	info->status.rates[3].idx,info->status.rates[3].count,info->status.rates[3].flags);
	}

// this is 11b stuff
//	if (info->flags&IEEE80211_TX_RC_USE_SHORT_PREAMBLE)
//		printk("%s openwifi_tx: WARNING IEEE80211_TX_RC_USE_SHORT_PREAMBLE\n", sdr_compatible_str);

	if (info->flags & IEEE80211_TX_CTL_ASSIGN_SEQ) {
		if (info->flags & IEEE80211_TX_CTL_FIRST_FRAGMENT)
			priv->seqno += 0x10;
		hdr->seq_ctrl &= cpu_to_le16(IEEE80211_SCTL_FRAG);
		hdr->seq_ctrl |= cpu_to_le16(priv->seqno);
	}
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
		if ((skb_new = skb_realloc_headroom(skb, LEN_PHY_HEADER)) == NULL) {
			printk("%s openwifi_tx: WARNING skb_realloc_headroom failed!\n", sdr_compatible_str);
			goto openwifi_tx_early_out;
		}
		if (skb->sk != NULL)
			skb_set_owner_w(skb_new, skb->sk);
		dev_kfree_skb(skb);
		skb = skb_new;
	}
	
	skb_push( skb, LEN_PHY_HEADER );
	rate_signal_value = calc_phy_header(rate_hw_value, len_mac_pdu+LEN_PHY_CRC, skb->data); //fill the phy header
	//make sure dma length is integer times of DDC_NUM_BYTE_PER_DMA_SYMBOL
	if (skb_tailroom(skb)<num_byte_pad) {
		printk("%s openwifi_tx: WARNING skb_tailroom(skb)<num_byte_pad!\n", sdr_compatible_str);
		goto openwifi_tx_early_out;
	}
	skb_put( skb, num_byte_pad );

	retry_limit_hw_value = (retry_limit_raw - 1)&0xF;
	dma_buf = skb->data;
	//(*((u16*)(dma_buf+6))) = priv->phy_tx_sn;
	//(*((u32*)(dma_buf+8))) = num_dma_byte;
	//(*((u32*)(dma_buf+12))) = num_byte_pad;

	cts_rate_signal_value = wifi_mcs_table_11b_force_up[cts_rate_hw_value];
	cts_reg = (((use_cts_protect|force_use_cts_protect)<<31)|(cts_use_traffic_rate<<30)|(cts_duration<<8)|(cts_rate_signal_value<<4)|rate_signal_value);
	dma_reg = ( (( ((priv->phy_tx_sn<<NUM_BIT_MAX_NUM_HW_QUEUE)|queue_idx) )<<18)|(retry_limit_hw_value<<14)|(pkt_need_ack<<13)|num_dma_symbol );
	spin_lock_irqsave(&priv->lock, flags); // from now on, we'd better avoid interrupt because wr/rd idx will matter

	//ring_len = (ring->bd_wr_idx>=ring->bd_rd_idx)?(ring->bd_wr_idx-ring->bd_rd_idx):(ring->bd_wr_idx+NUM_TX_BD-ring->bd_rd_idx);
	ring_len = ((ring->bd_wr_idx-ring->bd_rd_idx)&(NUM_TX_BD-1));
	ring_room_left = NUM_TX_BD - ring_len;
	if (ring_len>12)
		printk("%s openwifi_tx: WARNING ring len %d\n", sdr_compatible_str,ring_len);
//		printk("%s openwifi_tx: WARNING ring len %d HW fifo %d q %d\n", sdr_compatible_str,ring_len,tx_intf_api->TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_read()&0xFFFF, ((tx_intf_api->TX_INTF_REG_PHY_QUEUE_TX_SN_read())>>16)&0xFF );

	if (ring_room_left <= 2 && priv->tx_queue_stopped == false) {
		ieee80211_stop_queue(dev, prio);
		printk("%s openwifi_tx: WARNING ieee80211_stop_queue. ring_room_left %d!\n", sdr_compatible_str,ring_room_left);
		priv->tx_queue_stopped = true;
		spin_unlock_irqrestore(&priv->lock, flags);
		goto openwifi_tx_early_out;
	}

	/* We must be sure that tx_flags is written last because the HW
	 * looks at it to check if the rest of data is valid or not
	 */
	//wmb();
	// entry->flags = cpu_to_le32(tx_flags);
	/* We must be sure this has been written before followings HW
	 * register write, because this write will made the HW attempts
	 * to DMA the just-written data
	 */
	//wmb();

	//__skb_queue_tail(&ring->queue, skb);

//-------------------------fire skb DMA to hardware----------------------------------
	dma_mapping_addr = dma_map_single(priv->tx_chan->device->dev, dma_buf,
				 num_dma_byte, DMA_MEM_TO_DEV);

	if (dma_mapping_error(priv->tx_chan->device->dev,dma_mapping_addr)) {
		dev_err(priv->tx_chan->device->dev, "WARNING TX DMA mapping error\n");
		goto openwifi_tx_skb_drop_out;
	}

	sg_init_table(&(priv->tx_sg), 1);

	sg_dma_address( &(priv->tx_sg) ) = dma_mapping_addr;
	sg_dma_len( &(priv->tx_sg) ) = num_dma_byte;
	
	tx_intf_api->TX_INTF_REG_CTS_TOSELF_CONFIG_write(cts_reg);
	tx_intf_api->TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write(dma_reg);
	priv->txd = priv->tx_chan->device->device_prep_slave_sg(priv->tx_chan, &(priv->tx_sg),1,DMA_MEM_TO_DEV, DMA_CTRL_ACK | DMA_PREP_INTERRUPT, NULL);
	if (!(priv->txd)) {
		printk("%s openwifi_tx: WARNING device_prep_slave_sg %d\n", sdr_compatible_str, (u32)(priv->txd));
		goto openwifi_tx_after_dma_mapping;
	}

	//we use interrupt instead of dma callback
	priv->txd->callback = 0;
	priv->txd->callback_param = 0;
	priv->tx_cookie = priv->txd->tx_submit(priv->txd);

	if (dma_submit_error(priv->tx_cookie)) {
		printk("%s openwifi_tx: WARNING dma_submit_error(tx_cookie) %d\n", sdr_compatible_str, (u32)(priv->tx_cookie));
		goto openwifi_tx_after_dma_mapping;
	}

	// seems everything ok. let's mark this pkt in bd descriptor ring
	ring->bds[ring->bd_wr_idx].num_dma_byte=num_dma_byte;
	ring->bds[ring->bd_wr_idx].sn=priv->phy_tx_sn;
	// ring->bds[ring->bd_wr_idx].hw_queue_idx=queue_idx;
	// ring->bds[ring->bd_wr_idx].retry_limit=retry_limit_hw_value;
	// ring->bds[ring->bd_wr_idx].need_ack=pkt_need_ack;
	ring->bds[ring->bd_wr_idx].skb_linked = skb;
	ring->bds[ring->bd_wr_idx].dma_mapping_addr = dma_mapping_addr;

	ring->bd_wr_idx = ((ring->bd_wr_idx+1)&(NUM_TX_BD-1));
	priv->phy_tx_sn = ( (priv->phy_tx_sn+1)&MAX_PHY_TX_SN );

	dma_async_issue_pending(priv->tx_chan);

	spin_unlock_irqrestore(&priv->lock, flags);

	return;

openwifi_tx_after_dma_mapping:
	printk("%s openwifi_tx: WARNING openwifi_tx_after_dma_mapping phy_tx_sn %d queue %d\n", sdr_compatible_str,priv->phy_tx_sn,queue_idx);
	dma_unmap_single(priv->tx_chan->device->dev, dma_mapping_addr, num_dma_byte, DMA_MEM_TO_DEV);
	spin_unlock_irqrestore(&priv->lock, flags);

openwifi_tx_skb_drop_out:
	printk("%s openwifi_tx: WARNING openwifi_tx_skb_drop_out phy_tx_sn %d queue %d\n", sdr_compatible_str,priv->phy_tx_sn,queue_idx);
	spin_unlock_irqrestore(&priv->lock, flags);

openwifi_tx_early_out:
	dev_kfree_skb(skb);
	printk("%s openwifi_tx: WARNING openwifi_tx_early_out phy_tx_sn %d queue %d\n", sdr_compatible_str,priv->phy_tx_sn,queue_idx);
}

static int openwifi_start(struct ieee80211_hw *dev)
{
	struct openwifi_priv *priv = dev->priv;
	int ret, i, rssi_half_db_offset, agc_gain_delay;//rssi_half_db_th, 
	u32 reg;

	for (i=0; i<MAX_NUM_VIF; i++) {
		priv->vif[i] = NULL;
	}

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
	tx_intf_api->hw_init(priv->tx_intf_cfg,8,8);
	openofdm_tx_api->hw_init(priv->openofdm_tx_cfg);
	openofdm_rx_api->hw_init(priv->openofdm_rx_cfg);
	xpu_api->hw_init(priv->xpu_cfg);

	agc_gain_delay = 50; //samples
	rssi_half_db_offset = 150;
	xpu_api->XPU_REG_RSSI_DB_CFG_write(0x80000000|((rssi_half_db_offset<<16)|agc_gain_delay) );
	xpu_api->XPU_REG_RSSI_DB_CFG_write((~0x80000000)&((rssi_half_db_offset<<16)|agc_gain_delay) );
	
	openofdm_rx_api->OPENOFDM_RX_REG_POWER_THRES_write(0);
	// rssi_half_db_th = 87<<1; // -62dBm // will settup in runtime in _rf_set_channel
	// xpu_api->XPU_REG_LBT_TH_write(rssi_half_db_th); // set IQ rssi th step .5dB to xxx and enable it

	// // xpu_api->XPU_REG_CSMA_CFG_write(3); // cw_min
	// xpu_api->XPU_REG_CSMA_CFG_write(3);

	//xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( (1200<<16)|0 );//high 16bit 5GHz; low 16 bit 2.4GHz
	xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( ((1030-238)<<16)|0 );//high 16bit 5GHz; low 16 bit 2.4GHz (Attention, current tx core has around 1.19us starting delay that makes the ack fall behind 10us SIFS in 2.4GHz! Need to improve TX in 2.4GHz!)

	//xpu_api->XPU_REG_RECV_ACK_COUNT_TOP0_write( (((45+2+2)*200)<<16) | 400 );//2.4GHz
	//xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write( (((51+2+2)*200)<<16) | 400 );//5GHz

	// // value from openwifi-pre0 csma_test branch
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP0_write( (((45+2+2)*200)<<16) | 200 );//2.4GHz
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write( (((51+2+2)*200)<<16) | 200 );//5GHz

	tx_intf_api->TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write( ((16*200)<<16)|(10*200) );//high 16bit 5GHz; low 16 bit 2.4GHz
	
	//xpu_api->XPU_REG_BB_RF_DELAY_write(1020); // fine tuned value at 0.005us. old: dac-->ant port: 0.6us, 57 taps fir at 40MHz: 1.425us; round trip: 2*(0.6+1.425)=4.05us; 4.05*200=810
	xpu_api->XPU_REG_BB_RF_DELAY_write(975);//add .5us for slightly longer fir
	xpu_api->XPU_REG_MAC_ADDR_write(priv->mac_addr);

	xpu_api->XPU_REG_SLICE_COUNT_TOTAL0_write(50000-1); // total 50ms.
	xpu_api->XPU_REG_SLICE_COUNT_START0_write(0); //start 0ms
	xpu_api->XPU_REG_SLICE_COUNT_END0_write(50000-1); //end 50ms
	xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_write(50000-1); // total 50ms
	xpu_api->XPU_REG_SLICE_COUNT_START1_write(49000); //start 49ms
	xpu_api->XPU_REG_SLICE_COUNT_END1_write(50000-1); //end 50ms

	//xpu_api->XPU_REG_MAC_ADDR_HIGH_write( (*( (u16*)(priv->mac_addr + 4) )) );
	printk("%s openwifi_start: rx_intf_cfg %d openofdm_rx_cfg %d tx_intf_cfg %d openofdm_tx_cfg %d\n",sdr_compatible_str, priv->rx_intf_cfg, priv->openofdm_rx_cfg, priv->tx_intf_cfg, priv->openofdm_tx_cfg);
	printk("%s openwifi_start: rx_freq_offset_to_lo_MHz %d tx_freq_offset_to_lo_MHz %d\n",sdr_compatible_str, priv->rx_freq_offset_to_lo_MHz, priv->tx_freq_offset_to_lo_MHz);

	tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x30040); //disable tx interrupt
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
		//goto err_free_reg;
		//goto err_free_dev;
	}

	priv->tx_chan = dma_request_slave_channel(&(priv->pdev->dev), "tx_dma_mm2s");
	if (IS_ERR(priv->tx_chan)) {
		ret = PTR_ERR(priv->tx_chan);
		pr_err("%s openwifi_start: No Tx channel %d\n",sdr_compatible_str,ret);
		goto err_dma;
		//goto err_free_reg;
		//goto err_free_dev;
	}
	printk("%s openwifi_start: DMA channel setup successfully.\n",sdr_compatible_str);

	ret = openwifi_init_rx_ring(priv);
	if (ret) {
		printk("%s openwifi_start: openwifi_init_rx_ring ret %d\n", sdr_compatible_str,ret);
		goto err_free_rings;
	}

	priv->seqno=0;
	priv->phy_tx_sn=0;
	if ((ret = openwifi_init_tx_ring(priv))) {
		printk("%s openwifi_start: openwifi_init_tx_ring ret %d\n", sdr_compatible_str,ret);
		goto err_free_rings;
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
	tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x40); //enable tx interrupt
	rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(0); // release M AXIS
	xpu_api->XPU_REG_TSF_LOAD_VAL_write(0,0); // reset tsf timer

	//ieee80211_wake_queue(dev, 0);

normal_out:
	printk("%s openwifi_start: normal end\n", sdr_compatible_str);
	return 0;

err_free_rings:
	openwifi_free_rx_ring(priv);
	openwifi_free_tx_ring(priv);

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

	tx_intf_api->TX_INTF_REG_INTERRUPT_SEL_write(0x30040); //disable tx interrupt
	rx_intf_api->RX_INTF_REG_INTERRUPT_TEST_write(0x100); // disable fcs_valid by interrupt test mode
	rx_intf_api->RX_INTF_REG_M_AXIS_RST_write(1); // hold M AXIS in reset status

	for (i=0; i<MAX_NUM_VIF; i++) {
		priv->vif[i] = NULL;
	}

	openwifi_free_rx_ring(priv);
	openwifi_free_tx_ring(priv);

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

static int openwifi_conf_tx(struct ieee80211_hw *hw, struct ieee80211_vif *vif, u16 queue,
	      const struct ieee80211_tx_queue_params *params)
{
	printk("%s openwifi_conf_tx: WARNING [queue %d], aifs: %d, cw_min: %d, cw_max: %d, txop: %d\n",
		  sdr_compatible_str,queue,params->aifs,params->cw_min,params->cw_max,params->txop);
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
//	(*total_flags) |= FIF_ALLMULTI; //because we always pass all multicast (no matter it is for us or not) to upper layer

	filter_flag = (*total_flags);

	filter_flag = (filter_flag|UNICAST_FOR_US|BROADCAST_ALL_ONE|BROADCAST_ALL_ZERO);
	//filter_flag = (filter_flag|UNICAST_FOR_US|BROADCAST_ALL_ONE|BROADCAST_ALL_ZERO|MONITOR_ALL); // all pkt will be delivered to arm

	if (priv->vif[0]->type == NL80211_IFTYPE_MONITOR)
		filter_flag = (filter_flag|MONITOR_ALL);

	if ( (priv->vif[0]->type == NL80211_IFTYPE_STATION) && !(filter_flag&FIF_BCN_PRBRESP_PROMISC) )
		filter_flag = (filter_flag|MY_BEACON);

	if (priv->vif[0]->type == NL80211_IFTYPE_AP)
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
	u32 tmp=-1, reg_cat, reg_addr, reg_val, reg_addr_idx;

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
	case OPENWIFI_CMD_SET_ADDR0:
		if (!tb[OPENWIFI_ATTR_ADDR0])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_ADDR0]);
		printk("%s set openwifi slice0_target_mac_addr(low32) in hex: %08x\n", sdr_compatible_str, tmp);
		priv->dest_mac_addr_queue_map[0] = reverse32(tmp);
		return 0;
	case OPENWIFI_CMD_GET_ADDR0:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = reverse32(priv->dest_mac_addr_queue_map[0]);
		if (nla_put_u32(skb, OPENWIFI_ATTR_ADDR0, tmp))
			goto nla_put_failure;
		printk("%s get openwifi slice0_target_mac_addr(low32) in hex: %08x\n", sdr_compatible_str, tmp);
		return cfg80211_testmode_reply(skb);
	case OPENWIFI_CMD_SET_ADDR1:
		if (!tb[OPENWIFI_ATTR_ADDR1])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_ADDR1]);
		printk("%s set openwifi slice1_target_mac_addr(low32) in hex: %08x\n", sdr_compatible_str, tmp);
		priv->dest_mac_addr_queue_map[1] = reverse32(tmp);
		return 0;
	case OPENWIFI_CMD_GET_ADDR1:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = reverse32(priv->dest_mac_addr_queue_map[1]);
		if (nla_put_u32(skb, OPENWIFI_ATTR_ADDR1, tmp))
			goto nla_put_failure;
		printk("%s get openwifi slice1_target_mac_addr(low32) in hex: %08x\n", sdr_compatible_str, tmp);
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_TOTAL0:
		if (!tb[OPENWIFI_ATTR_SLICE_TOTAL0])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_TOTAL0]);
		printk("%s set SLICE_TOTAL0(duration) to %d usec\n", sdr_compatible_str, tmp);
		xpu_api->XPU_REG_SLICE_COUNT_TOTAL0_write(tmp);
		return 0;
	case OPENWIFI_CMD_GET_SLICE_TOTAL0:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_TOTAL0_read());
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_TOTAL0, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_START0:
		if (!tb[OPENWIFI_ATTR_SLICE_START0])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_START0]);
		printk("%s set SLICE_START0(duration) to %d usec\n", sdr_compatible_str, tmp);
		xpu_api->XPU_REG_SLICE_COUNT_START0_write(tmp);
		return 0;
	case OPENWIFI_CMD_GET_SLICE_START0:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_START0_read());
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_START0, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_END0:
		if (!tb[OPENWIFI_ATTR_SLICE_END0])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_END0]);
		printk("%s set SLICE_END0(duration) to %d usec\n", sdr_compatible_str, tmp);
		xpu_api->XPU_REG_SLICE_COUNT_END0_write(tmp);
		return 0;
	case OPENWIFI_CMD_GET_SLICE_END0:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_END0_read());
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_END0, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_TOTAL1:
		if (!tb[OPENWIFI_ATTR_SLICE_TOTAL1])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_TOTAL1]);
		printk("%s set SLICE_TOTAL1(duration) to %d usec\n", sdr_compatible_str, tmp);
		xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_write(tmp);
		return 0;
	case OPENWIFI_CMD_GET_SLICE_TOTAL1:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_read());
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_TOTAL1, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_START1:
		if (!tb[OPENWIFI_ATTR_SLICE_START1])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_START1]);
		printk("%s set SLICE_START1(duration) to %d usec\n", sdr_compatible_str, tmp);
		xpu_api->XPU_REG_SLICE_COUNT_START1_write(tmp);
		return 0;
	case OPENWIFI_CMD_GET_SLICE_START1:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_START1_read());
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_START1, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

	case OPENWIFI_CMD_SET_SLICE_END1:
		if (!tb[OPENWIFI_ATTR_SLICE_END1])
			return -EINVAL;
		tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_END1]);
		printk("%s set SLICE_END1(duration) to %d usec\n", sdr_compatible_str, tmp);
		xpu_api->XPU_REG_SLICE_COUNT_END1_write(tmp);
		return 0;
	case OPENWIFI_CMD_GET_SLICE_END1:
		skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
		if (!skb)
			return -ENOMEM;
		tmp = (xpu_api->XPU_REG_SLICE_COUNT_END1_read());
		if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_END1, tmp))
			goto nla_put_failure;
		return cfg80211_testmode_reply(skb);

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
			priv->drv_rx_reg_val[reg_addr_idx]=reg_val;
			if (reg_addr_idx==1) {
				if (reg_val==0)
					priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_0MHZ_ANT0;
				else
					priv->rx_intf_cfg = RX_INTF_BW_20MHZ_AT_0MHZ_ANT1;

				priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
				//priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];
			}
		}
		else if (reg_cat==8) {
			priv->drv_tx_reg_val[reg_addr_idx]=reg_val;
			if (reg_addr_idx==1) {
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
		}
		else if (reg_cat==9) {
			priv->drv_xpu_reg_val[reg_addr_idx]=reg_val;
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
			if (reg_addr_idx==1) {
				priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
				//priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];

				if (priv->rx_intf_cfg == RX_INTF_BW_20MHZ_AT_0MHZ_ANT0)
					priv->drv_rx_reg_val[reg_addr_idx]=0;
				else if	(priv->rx_intf_cfg == RX_INTF_BW_20MHZ_AT_0MHZ_ANT1)
					priv->drv_rx_reg_val[reg_addr_idx]=1;
			}
			tmp = priv->drv_rx_reg_val[reg_addr_idx];
		}
		else if (reg_cat==8) {
			if (reg_addr_idx==1) {
				//priv->rx_freq_offset_to_lo_MHz = rx_intf_fo_mapping[priv->rx_intf_cfg];
				priv->tx_freq_offset_to_lo_MHz = tx_intf_fo_mapping[priv->tx_intf_cfg];
				if (priv->tx_intf_cfg == TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0)
					priv->drv_tx_reg_val[reg_addr_idx]=0;
				else if	(priv->tx_intf_cfg == TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1)
					priv->drv_tx_reg_val[reg_addr_idx]=1;
			}
			tmp = priv->drv_tx_reg_val[reg_addr_idx];
		}
		else if (reg_cat==9) {
			tmp = priv->drv_xpu_reg_val[reg_addr_idx];
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
	const char *chip_name;
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

	// //-------------find ad9361-phy driver for lo/channel control---------------
	priv->actual_rx_lo = 0;
	tmp_dev = bus_find_device( &spi_bus_type, NULL, "ad9361-phy", custom_match_spi_dev );
	if (!tmp_dev) {
		printk(KERN_ERR "%s find_dev ad9361-phy failed\n",sdr_compatible_str);
		err = -ENOMEM;
		goto err_free_dev;
	}
	priv->ad9361_phy = ad9361_spi_to_phy((struct spi_device*)tmp_dev);
	if (!(priv->ad9361_phy)) {
		printk(KERN_ERR "%s ad9361_spi_to_phy failed\n",sdr_compatible_str);
		err = -ENOMEM;
		goto err_free_dev;
	}

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
		priv->tx_intf_cfg = TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1;
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
	tx_intf_api->hw_init(priv->tx_intf_cfg,8,8);
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
	dev->wiphy->bands[NL80211_BAND_2GHZ] = &(priv->band_2GHz);

	priv->band_5GHz.band = NL80211_BAND_5GHZ;
	priv->band_5GHz.channels = priv->channels_5GHz;
	priv->band_5GHz.n_channels = ARRAY_SIZE(priv->channels_5GHz);
	priv->band_5GHz.bitrates = priv->rates_5GHz;
	priv->band_5GHz.n_bitrates = ARRAY_SIZE(priv->rates_5GHz);
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
	dev->queues = 1;

	ieee80211_hw_set(dev, SIGNAL_DBM);

	wiphy_ext_feature_set(dev->wiphy, NL80211_EXT_FEATURE_CQM_RSSI_LIST);

	priv->rf = &ad9361_rf_ops;

	memset(priv->dest_mac_addr_queue_map,0,sizeof(priv->dest_mac_addr_queue_map));

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
	// tmp_dev = bus_find_device( &platform_bus_type, NULL, "leds", custom_match_platform_dev ); //leds is the name in devicetree, not "compatiable" field
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
		pr_info("%s openwifi_dev_remove: dev %d\n", sdr_compatible_str, (u32)dev);
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
