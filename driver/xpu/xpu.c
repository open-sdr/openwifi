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
#include <linux/delay.h>
#include <net/mac80211.h>

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

static inline void XPU_REG_MULTI_RST_write(u32 Data) {
	reg_write(XPU_REG_MULTI_RST_ADDR, Data);
}

static inline u32 XPU_REG_MULTI_RST_read(void){
	return reg_read(XPU_REG_MULTI_RST_ADDR);
}

static inline void XPU_REG_SRC_SEL_write(u32 Data) {
	reg_write(XPU_REG_SRC_SEL_ADDR, Data);
}

static inline u32 XPU_REG_SRC_SEL_read(void){
	return reg_read(XPU_REG_SRC_SEL_ADDR);
}

static inline void XPU_REG_RECV_ACK_COUNT_TOP0_write(u32 Data) {
	reg_write(XPU_REG_RECV_ACK_COUNT_TOP0_ADDR, Data);
}

static inline u32 XPU_REG_RECV_ACK_COUNT_TOP0_read(void){
	return reg_read(XPU_REG_RECV_ACK_COUNT_TOP0_ADDR);
}

static inline void XPU_REG_RECV_ACK_COUNT_TOP1_write(u32 Data) {
	reg_write(XPU_REG_RECV_ACK_COUNT_TOP1_ADDR, Data);
}

static inline u32 XPU_REG_RECV_ACK_COUNT_TOP1_read(void){
	return reg_read(XPU_REG_RECV_ACK_COUNT_TOP1_ADDR);
}

static inline void XPU_REG_SEND_ACK_WAIT_TOP_write(u32 Data) {
	reg_write(XPU_REG_SEND_ACK_WAIT_TOP_ADDR, Data);
}

static inline u32 XPU_REG_SEND_ACK_WAIT_TOP_read(void){
	return reg_read(XPU_REG_SEND_ACK_WAIT_TOP_ADDR);
}

static inline void XPU_REG_FILTER_FLAG_write(u32 Data) {
	reg_write(XPU_REG_FILTER_FLAG_ADDR, Data);
}

static inline u32 XPU_REG_FILTER_FLAG_read(void){
	return reg_read(XPU_REG_FILTER_FLAG_ADDR);
}

static inline void XPU_REG_CTS_TO_RTS_CONFIG_write(u32 Data) {
	reg_write(XPU_REG_CTS_TO_RTS_CONFIG_ADDR, Data);
}

static inline u32 XPU_REG_CTS_TO_RTS_CONFIG_read(void){
	return reg_read(XPU_REG_CTS_TO_RTS_CONFIG_ADDR);
}

static inline void XPU_REG_MAC_ADDR_LOW_write(u32 Data) {
	reg_write(XPU_REG_MAC_ADDR_LOW_ADDR, Data);
}

static inline u32 XPU_REG_MAC_ADDR_LOW_read(void){
	return reg_read(XPU_REG_MAC_ADDR_LOW_ADDR);
}

static inline void XPU_REG_MAC_ADDR_HIGH_write(u32 Data) {
	reg_write(XPU_REG_MAC_ADDR_HIGH_ADDR, Data);
}

static inline u32 XPU_REG_MAC_ADDR_HIGH_read(void){
	return reg_read(XPU_REG_MAC_ADDR_HIGH_ADDR);
}

static inline void XPU_REG_BSSID_FILTER_LOW_write(u32 Data) {
	reg_write(XPU_REG_BSSID_FILTER_LOW_ADDR, Data);
}

static inline u32 XPU_REG_BSSID_FILTER_LOW_read(void){
	return reg_read(XPU_REG_BSSID_FILTER_LOW_ADDR);
}

static inline void XPU_REG_BSSID_FILTER_HIGH_write(u32 Data) {
	reg_write(XPU_REG_BSSID_FILTER_HIGH_ADDR, Data);
}

static inline u32 XPU_REG_BSSID_FILTER_HIGH_read(void){
	return reg_read(XPU_REG_BSSID_FILTER_HIGH_ADDR);
}

static inline void XPU_REG_BAND_CHANNEL_write(u32 Data) {
	reg_write(XPU_REG_BAND_CHANNEL_ADDR, Data);
}

static inline u32 XPU_REG_BAND_CHANNEL_read(void){
	return reg_read(XPU_REG_BAND_CHANNEL_ADDR);
}

static inline void XPU_REG_DIFS_ADVANCE_write(u32 Data) {
	reg_write(XPU_REG_DIFS_ADVANCE_ADDR, Data);
}

static inline u32 XPU_REG_DIFS_ADVANCE_read(void){
	return reg_read(XPU_REG_DIFS_ADVANCE_ADDR);
}

static inline void XPU_REG_FORCE_IDLE_MISC_write(u32 Data) {
	reg_write(XPU_REG_FORCE_IDLE_MISC_ADDR, Data);
}

static inline u32 XPU_REG_FORCE_IDLE_MISC_read(void){
	return reg_read(XPU_REG_FORCE_IDLE_MISC_ADDR);
}

static inline u32 XPU_REG_TSF_RUNTIME_VAL_LOW_read(void){
	return reg_read(XPU_REG_TSF_RUNTIME_VAL_LOW_ADDR);
}

static inline u32 XPU_REG_TSF_RUNTIME_VAL_HIGH_read(void){
	return reg_read(XPU_REG_TSF_RUNTIME_VAL_HIGH_ADDR);
}

static inline void XPU_REG_TSF_LOAD_VAL_LOW_write(u32 value){
	reg_write(XPU_REG_TSF_LOAD_VAL_LOW_ADDR, value);
}

static inline void XPU_REG_TSF_LOAD_VAL_HIGH_write(u32 value){
	reg_write(XPU_REG_TSF_LOAD_VAL_HIGH_ADDR, value);
}

static inline void XPU_REG_TSF_LOAD_VAL_write(u32 high_value, u32 low_value){
	XPU_REG_TSF_LOAD_VAL_LOW_write(low_value);
	XPU_REG_TSF_LOAD_VAL_HIGH_write(high_value|0x80000000); // msb high
	XPU_REG_TSF_LOAD_VAL_HIGH_write(high_value&(~0x80000000)); // msb low
}

static inline void XPU_REG_LBT_TH_write(u32 value) {
	reg_write(XPU_REG_LBT_TH_ADDR, value);
}

static inline u32 XPU_REG_RSSI_DB_CFG_read(void){
	return reg_read(XPU_REG_RSSI_DB_CFG_ADDR);
}

static inline void XPU_REG_RSSI_DB_CFG_write(u32 Data) {
	reg_write(XPU_REG_RSSI_DB_CFG_ADDR, Data);
}

static inline u32 XPU_REG_LBT_TH_read(void){
	return reg_read(XPU_REG_LBT_TH_ADDR);
}

static inline void XPU_REG_CSMA_DEBUG_write(u32 value){
	reg_write(XPU_REG_CSMA_DEBUG_ADDR, value);
}

static inline u32 XPU_REG_CSMA_DEBUG_read(void){
	return reg_read(XPU_REG_CSMA_DEBUG_ADDR);
}

static inline void XPU_REG_CSMA_CFG_write(u32 value){
	reg_write(XPU_REG_CSMA_CFG_ADDR, value);
}

static inline u32 XPU_REG_CSMA_CFG_read(void){
	return reg_read(XPU_REG_CSMA_CFG_ADDR);
}

static inline void XPU_REG_SLICE_COUNT_TOTAL_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_TOTAL_ADDR, value);
}
static inline void XPU_REG_SLICE_COUNT_START_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_START_ADDR, value);
}
static inline void XPU_REG_SLICE_COUNT_END_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_END_ADDR, value);
}


static inline u32 XPU_REG_SLICE_COUNT_TOTAL_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_TOTAL_ADDR);
}
static inline u32 XPU_REG_SLICE_COUNT_START_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_START_ADDR);
}
static inline u32 XPU_REG_SLICE_COUNT_END_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_END_ADDR);
}

static inline void XPU_REG_BB_RF_DELAY_write(u32 value){
	reg_write(XPU_REG_BB_RF_DELAY_ADDR, value);
}

static inline void XPU_REG_ACK_CTL_MAX_NUM_RETRANS_write(u32 value){
	reg_write(XPU_REG_ACK_CTL_MAX_NUM_RETRANS_ADDR, value);
}
static inline u32 XPU_REG_ACK_CTL_MAX_NUM_RETRANS_read(void){
	return reg_read(XPU_REG_ACK_CTL_MAX_NUM_RETRANS_ADDR);
}

static inline void XPU_REG_AMPDU_ACTION_write(u32 Data) {
	reg_write(XPU_REG_AMPDU_ACTION_ADDR, Data);
}

static inline u32 XPU_REG_AMPDU_ACTION_read(void){
	return reg_read(XPU_REG_AMPDU_ACTION_ADDR);
}

static inline void XPU_REG_SPI_DISABLE_write(u32 Data) {
	reg_write(XPU_REG_SPI_DISABLE_ADDR, Data);
}

static inline u32 XPU_REG_SPI_DISABLE_read(void){
	return reg_read(XPU_REG_SPI_DISABLE_ADDR);
}

static inline void XPU_REG_MAC_ADDR_write(u8 *mac_addr) {//, u32 en_flag){
	XPU_REG_MAC_ADDR_LOW_write( *( (u32*)(mac_addr) ) );
	XPU_REG_MAC_ADDR_HIGH_write( *( (u16*)(mac_addr + 4) ) );
	#if 0
	if (en_flag) {
		XPU_REG_MAC_ADDR_HIGH_write( (*( (u16*)(mac_addr + 4) )) | 0x80000000 ); // 0x80000000 by default we turn on mac addr filter
	} else {
		XPU_REG_MAC_ADDR_HIGH_write( (*( (u16*)(mac_addr + 4) )) & 0x7FFFFFFF );
	}
	#endif
}

static const struct of_device_id dev_of_ids[] = {
	{ .compatible = "sdr,xpu", },
	{}
};
MODULE_DEVICE_TABLE(of, dev_of_ids);

static struct xpu_driver_api xpu_driver_api_inst;
struct xpu_driver_api *xpu_api = &xpu_driver_api_inst;
EXPORT_SYMBOL(xpu_api);

static inline u32 hw_init(enum xpu_mode mode){
	int err=0, i, rssi_half_db_th, rssi_half_db_offset, agc_gain_delay;
	// u32 filter_flag = 0;

	printk("%s hw_init mode %d\n", xpu_compatible_str, mode);

	//rst
	for (i=0;i<8;i++)
		xpu_api->XPU_REG_MULTI_RST_write(0);
	for (i=0;i<32;i++)
		xpu_api->XPU_REG_MULTI_RST_write(0xFFFFFFFF);
	for (i=0;i<8;i++)
		xpu_api->XPU_REG_MULTI_RST_write(0);

	// http://www.studioreti.it/slide/802-11-Frame_E_C.pdf
	// https://mrncciew.com/2014/10/14/cwap-802-11-phy-ppdu/
	// https://mrncciew.com/2014/09/27/cwap-mac-header-frame-control/
	// https://mrncciew.com/2014/10/25/cwap-mac-header-durationid/
	// https://mrncciew.com/2014/11/01/cwap-mac-header-sequence-control/
	// https://witestlab.poly.edu/blog/802-11-wireless-lan-2/
	// phy_rx byte idx: 
	// 5(3 sig + 2 service), -- PHY
	// 2 frame control, 2 duration/conn ID, --MAC PDU
	// 6 receiver address, 6 destination address, 6 transmitter address
	// 2 sequence control
	// 6 source address
	// reg_val = 5 + 0;
	// xpu_api->XPU_REG_PHY_RX_PKT_READ_OFFSET_write(reg_val);
	// printk("%s hw_init XPU_REG_PHY_RX_PKT_READ_OFFSET_write %d\n", xpu_compatible_str, reg_val);

	// by default turn off filter, because all register are zeros
	// let's filter out packet according to: enum ieee80211_filter_flags at: https://www.kernel.org/doc/html/v4.9/80211/mac80211.html
	#if 0 // define in FPGA
    localparam [13:0]   FIF_ALLMULTI =           14b00000000000010, //get all mac addr like 01:00:5E:xx:xx:xx and 33:33:xx:xx:xx:xx through to ARM
                        FIF_FCSFAIL =            14b00000000000100, //not support
                        FIF_PLCPFAIL =           14b00000000001000, //not support
                        FIF_BCN_PRBRESP_PROMISC= 14b00000000010000, 
                        FIF_CONTROL =            14b00000000100000,
                        FIF_OTHER_BSS =          14b00000001000000, 
                        FIF_PSPOLL =             14b00000010000000,
                        FIF_PROBE_REQ =          14b00000100000000,
                        UNICAST_FOR_US =         14b00001000000000,
                        BROADCAST_ALL_ONE =      14b00010000000000,
                        BROADCAST_ALL_ZERO =     14b00100000000000,
                        MY_BEACON          =     14b01000000000000,
                        MONITOR_ALL =            14b10000000000000;
	#endif
	
  // Remove XPU_REG_FILTER_FLAG_write to avoid hw_init call in openwifi_start causing inconsistency
  // filter_flag = (FIF_ALLMULTI|FIF_FCSFAIL|FIF_PLCPFAIL|FIF_BCN_PRBRESP_PROMISC|FIF_CONTROL|FIF_OTHER_BSS|FIF_PSPOLL|FIF_PROBE_REQ|UNICAST_FOR_US|BROADCAST_ALL_ONE|BROADCAST_ALL_ZERO|MY_BEACON|MONITOR_ALL);
	// xpu_api->XPU_REG_FILTER_FLAG_write(filter_flag);

	xpu_api->XPU_REG_CTS_TO_RTS_CONFIG_write(0xB<<16);//6M 1011:0xB

	// after send data frame wait for ACK, this will be set in real time in function ad9361_rf_set_channel
	// xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write( (((51+2)*10)<<16) | 10 ); // high 16 bits to cover sig valid of ACK packet, low 16 bits is adjustment of fcs valid waiting time.  let's add 2us for those device that is really "slow"!
	// xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( 6*10 ); // +6 = 16us for 5GHz

	//xpu_api->XPU_REG_ACK_CTL_MAX_NUM_RETRANS_write(3); // if this > 0, it will override mac80211 set value, and set static retransmission limit
	
	// From CMW measurement: lo up 1us before the packet; lo down 0.4us after the packet/RF port switches 1.2us before and 0.2us after
	xpu_api->XPU_REG_BB_RF_DELAY_write((16<<24)|(0<<16)|(26<<8)|9); // calibrated by ila and spectrum analyzer (trigger mode)

	// setup time schedule of all queues. all time open.
	for (i=0; i<4; i++) {
		xpu_api->XPU_REG_SLICE_COUNT_TOTAL_write((i<<20)|16);//total 16us
		xpu_api->XPU_REG_SLICE_COUNT_START_write((i<<20)|0); //start 0us
		xpu_api->XPU_REG_SLICE_COUNT_END_write((i<<20)|16);  //end   16us
	}

	// all slice sync rest
	xpu_api->XPU_REG_MULTI_RST_write(1<<7); //bit7 reset the counter for all queues at the same time
	xpu_api->XPU_REG_MULTI_RST_write(0<<7); 
	
	switch(mode)
	{
		case XPU_TEST:
			printk("%s hw_init mode XPU_TEST\n", xpu_compatible_str);
			break;

		case XPU_NORMAL:
			printk("%s hw_init mode XPU_NORMAL\n", xpu_compatible_str);
			break;

		default:
			printk("%s hw_init mode %d is wrong!\n", xpu_compatible_str, mode);
			err=1;
	}
  // Remove this XPU_REG_BAND_CHANNEL_write in xpu.c, because
  // 1. the 44 for channel field is out dated. Now the channel actually should be frequency in MHz
  // 2. PROBLEM! this hw_init call in openwifi_start will cause lossing consistency between XPU register and
  // (priv->use_short_slot<<24)|(priv->band<<16)|(priv->actual_rx_lo)
	// xpu_api->XPU_REG_BAND_CHANNEL_write((false<<24)|(BAND_5_8GHZ<<16)|44);//use_short_slot==false; 5.8GHz; channel 44 -- default setting to sync with priv->band/channel/use_short_slot

	agc_gain_delay = 39; //samples
	rssi_half_db_offset = 75<<1;
	xpu_api->XPU_REG_RSSI_DB_CFG_write(0x80000000|((rssi_half_db_offset<<16)|agc_gain_delay) );
	xpu_api->XPU_REG_RSSI_DB_CFG_write((~0x80000000)&((rssi_half_db_offset<<16)|agc_gain_delay) );
	
	//rssi_half_db_th = 70<<1; // with splitter
	rssi_half_db_th = 87<<1; // -62dBm
	xpu_api->XPU_REG_LBT_TH_write(rssi_half_db_th); // set IQ rssi th step .5dB to xxx and enable it

  // control the duration to force ch_idle after decoding a packet due to imperfection of agc and signals
  // (1<<26) to disable eifs_trigger_by_last_tx_fail by default (standard does not ask so)
	xpu_api->XPU_REG_FORCE_IDLE_MISC_write((1<<26)|75);

	//xpu_api->XPU_REG_CSMA_DEBUG_write((1<<31)|(20<<24)|(4<<19)|(3<<14)|(10<<7)|(5));
	xpu_api->XPU_REG_CSMA_DEBUG_write(0);
	
	// xpu_api->XPU_REG_CSMA_CFG_write(268435459);  // Linux will do config for each queue via openwifi_conf_tx
	// xpu_api->XPU_REG_CSMA_CFG_write(0xe0000000); // Linux will do config for each queue via openwifi_conf_tx

//	// ------- assume 2.4 and 5GHz have the same SIFS (6us signal extension) --------
	xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( ((16+25+7-3+8-2)<<16)|((16+25+7-3+8-2)<<0) ); //+7 according to the ACK timing check by IQ sample: iq_ack_timing.md. -3 after Colvin LLR. +8 after new faster dac intf. -2 calibration in Oct. 2024
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP0_write( (1<<31) | (((51+2+2)*10 + 15)<<16) | (10+3) );//2.4GHz. extra 300 clocks are needed when rx core fall into fake ht detection phase (rx mcs 6M). +3 after Colvin LLR
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write( (1<<31) | (((51+2+2)*10 + 15)<<16) | (10+3) );//5GHz. extra 300 clocks are needed when rx core fall into fake ht detection phase (rx mcs 6M). +3 after Colvin LLR
//	// ------- assume 2.4 and 5GHz have different SIFS --------
	// xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( ((16+23)<<16)|(0+23) );
	// xpu_api->XPU_REG_RECV_ACK_COUNT_TOP0_write( (1<<31) | (((45+2+2)*10 + 15)<<16) | 10 );//2.4GHz. extra 300 clocks are needed when rx core fall into fake ht detection phase (rx mcs 6M)
	// xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write( (1<<31) | (((51+2+2)*10 + 15)<<16) | 10 );//5GHz. extra 300 clocks are needed when rx core fall into fake ht detection phase (rx mcs 6M)

	xpu_api->XPU_REG_DIFS_ADVANCE_write((OPENWIFI_MAX_SIGNAL_LEN_TH<<16)|2); //us. bit31~16 max pkt length threshold

	printk("%s hw_init err %d\n", xpu_compatible_str, err);
	return(err);
}

static int dev_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource *io;
	u32 test_us0, test_us1, test_us2;
	int err=1;

	printk("\n");

	if (np) {
		const struct of_device_id *match;

		match = of_match_node(dev_of_ids, np);
		if (match) {
			printk("%s dev_probe match!\n", xpu_compatible_str);
			err = 0;
		}
	}

	if (err)
		return err;

	xpu_api->hw_init=hw_init;

	xpu_api->reg_read=reg_read;
	xpu_api->reg_write=reg_write;

	xpu_api->XPU_REG_MULTI_RST_write=XPU_REG_MULTI_RST_write;
	xpu_api->XPU_REG_MULTI_RST_read=XPU_REG_MULTI_RST_read;
	xpu_api->XPU_REG_SRC_SEL_write=XPU_REG_SRC_SEL_write;
	xpu_api->XPU_REG_SRC_SEL_read=XPU_REG_SRC_SEL_read;

	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP0_write=XPU_REG_RECV_ACK_COUNT_TOP0_write;
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP0_read=XPU_REG_RECV_ACK_COUNT_TOP0_read;
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write=XPU_REG_RECV_ACK_COUNT_TOP1_write;
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_read=XPU_REG_RECV_ACK_COUNT_TOP1_read;
	xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write=XPU_REG_SEND_ACK_WAIT_TOP_write;
	xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_read=XPU_REG_SEND_ACK_WAIT_TOP_read;
	xpu_api->XPU_REG_MAC_ADDR_LOW_write=XPU_REG_MAC_ADDR_LOW_write;
	xpu_api->XPU_REG_MAC_ADDR_LOW_read=XPU_REG_MAC_ADDR_LOW_read;
	xpu_api->XPU_REG_MAC_ADDR_HIGH_write=XPU_REG_MAC_ADDR_HIGH_write;
	xpu_api->XPU_REG_MAC_ADDR_HIGH_read=XPU_REG_MAC_ADDR_HIGH_read;

	xpu_api->XPU_REG_FILTER_FLAG_write=XPU_REG_FILTER_FLAG_write;
	xpu_api->XPU_REG_FILTER_FLAG_read=XPU_REG_FILTER_FLAG_read;
	xpu_api->XPU_REG_CTS_TO_RTS_CONFIG_write=XPU_REG_CTS_TO_RTS_CONFIG_write;
	xpu_api->XPU_REG_CTS_TO_RTS_CONFIG_read=XPU_REG_CTS_TO_RTS_CONFIG_read;
	xpu_api->XPU_REG_BSSID_FILTER_LOW_write=XPU_REG_BSSID_FILTER_LOW_write;
	xpu_api->XPU_REG_BSSID_FILTER_LOW_read=XPU_REG_BSSID_FILTER_LOW_read;
	xpu_api->XPU_REG_BSSID_FILTER_HIGH_write=XPU_REG_BSSID_FILTER_HIGH_write;
	xpu_api->XPU_REG_BSSID_FILTER_HIGH_read=XPU_REG_BSSID_FILTER_HIGH_read;

	xpu_api->XPU_REG_BAND_CHANNEL_write=XPU_REG_BAND_CHANNEL_write;
	xpu_api->XPU_REG_BAND_CHANNEL_read=XPU_REG_BAND_CHANNEL_read;

	xpu_api->XPU_REG_DIFS_ADVANCE_write=XPU_REG_DIFS_ADVANCE_write;
	xpu_api->XPU_REG_DIFS_ADVANCE_read=XPU_REG_DIFS_ADVANCE_read;

	xpu_api->XPU_REG_FORCE_IDLE_MISC_write=XPU_REG_FORCE_IDLE_MISC_write;
	xpu_api->XPU_REG_FORCE_IDLE_MISC_read=XPU_REG_FORCE_IDLE_MISC_read;

	xpu_api->XPU_REG_TSF_RUNTIME_VAL_LOW_read=XPU_REG_TSF_RUNTIME_VAL_LOW_read;
	xpu_api->XPU_REG_TSF_RUNTIME_VAL_HIGH_read=XPU_REG_TSF_RUNTIME_VAL_HIGH_read;
	xpu_api->XPU_REG_TSF_LOAD_VAL_LOW_write=XPU_REG_TSF_LOAD_VAL_LOW_write;
	xpu_api->XPU_REG_TSF_LOAD_VAL_HIGH_write=XPU_REG_TSF_LOAD_VAL_HIGH_write;
	xpu_api->XPU_REG_TSF_LOAD_VAL_write=XPU_REG_TSF_LOAD_VAL_write;
	
	xpu_api->XPU_REG_LBT_TH_write=XPU_REG_LBT_TH_write;
	xpu_api->XPU_REG_LBT_TH_read=XPU_REG_LBT_TH_read;

	xpu_api->XPU_REG_RSSI_DB_CFG_read=XPU_REG_RSSI_DB_CFG_read;
	xpu_api->XPU_REG_RSSI_DB_CFG_write=XPU_REG_RSSI_DB_CFG_write;

	xpu_api->XPU_REG_CSMA_DEBUG_write=XPU_REG_CSMA_DEBUG_write;
	xpu_api->XPU_REG_CSMA_DEBUG_read=XPU_REG_CSMA_DEBUG_read;

	xpu_api->XPU_REG_CSMA_CFG_write=XPU_REG_CSMA_CFG_write;
	xpu_api->XPU_REG_CSMA_CFG_read=XPU_REG_CSMA_CFG_read;

	xpu_api->XPU_REG_SLICE_COUNT_TOTAL_write=XPU_REG_SLICE_COUNT_TOTAL_write;
	xpu_api->XPU_REG_SLICE_COUNT_START_write=XPU_REG_SLICE_COUNT_START_write;
	xpu_api->XPU_REG_SLICE_COUNT_END_write=XPU_REG_SLICE_COUNT_END_write;

	xpu_api->XPU_REG_SLICE_COUNT_TOTAL_read=XPU_REG_SLICE_COUNT_TOTAL_read;
	xpu_api->XPU_REG_SLICE_COUNT_START_read=XPU_REG_SLICE_COUNT_START_read;
	xpu_api->XPU_REG_SLICE_COUNT_END_read=XPU_REG_SLICE_COUNT_END_read;

	xpu_api->XPU_REG_BB_RF_DELAY_write=XPU_REG_BB_RF_DELAY_write;

	xpu_api->XPU_REG_ACK_CTL_MAX_NUM_RETRANS_write=XPU_REG_ACK_CTL_MAX_NUM_RETRANS_write;
	xpu_api->XPU_REG_ACK_CTL_MAX_NUM_RETRANS_read=XPU_REG_ACK_CTL_MAX_NUM_RETRANS_read;

	xpu_api->XPU_REG_AMPDU_ACTION_write=XPU_REG_AMPDU_ACTION_write;
	xpu_api->XPU_REG_AMPDU_ACTION_read=XPU_REG_AMPDU_ACTION_read;

	xpu_api->XPU_REG_SPI_DISABLE_write=XPU_REG_SPI_DISABLE_write;
	xpu_api->XPU_REG_SPI_DISABLE_read=XPU_REG_SPI_DISABLE_read;	

	xpu_api->XPU_REG_MAC_ADDR_write=XPU_REG_MAC_ADDR_write;

	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base_addr = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(base_addr))
		return PTR_ERR(base_addr);

	printk("%s dev_probe io start 0x%08x end 0x%08x name %s flags 0x%08x desc 0x%08x\n", xpu_compatible_str,io->start,io->end,io->name,(u32)io->flags,(u32)io->desc);
	printk("%s dev_probe base_addr 0x%08x\n", xpu_compatible_str,(u32)base_addr);
	printk("%s dev_probe xpu_driver_api_inst 0x%08x\n", xpu_compatible_str, (u32)&xpu_driver_api_inst);
	printk("%s dev_probe             xpu_api 0x%08x\n", xpu_compatible_str, (u32)xpu_api);

	printk("%s dev_probe reset tsf timer\n", xpu_compatible_str);
	xpu_api->XPU_REG_TSF_LOAD_VAL_write(0,0);
	test_us0 = xpu_api->XPU_REG_TSF_RUNTIME_VAL_LOW_read();
	mdelay(33);
	test_us1 = xpu_api->XPU_REG_TSF_RUNTIME_VAL_LOW_read();
	mdelay(67);
	test_us2 = xpu_api->XPU_REG_TSF_RUNTIME_VAL_LOW_read();
	printk("%s dev_probe XPU_REG_TSF_RUNTIME_VAL_LOW_read %d %d %dus\n", xpu_compatible_str, test_us0, test_us1, test_us2);

	printk("%s dev_probe succeed!\n", xpu_compatible_str);

	err = hw_init(XPU_NORMAL);

	return err;
}

static int dev_remove(struct platform_device *pdev)
{
	printk("\n");

	printk("%s dev_remove base_addr 0x%08x\n", xpu_compatible_str,(u32)base_addr);
	printk("%s dev_remove xpu_driver_api_inst 0x%08x\n", xpu_compatible_str, (u32)&xpu_driver_api_inst);
	printk("%s dev_remove             xpu_api 0x%08x\n", xpu_compatible_str, (u32)xpu_api);

	printk("%s dev_remove succeed!\n", xpu_compatible_str);
	return 0;
}

static struct platform_driver dev_driver = {
	.driver = {
		.name = "sdr,xpu",
		.owner = THIS_MODULE,
		.of_match_table = dev_of_ids,
	},
	.probe = dev_probe,
	.remove = dev_remove,
};

module_platform_driver(dev_driver);

MODULE_AUTHOR("Xianjun Jiao");
MODULE_DESCRIPTION("sdr,xpu");
MODULE_LICENSE("GPL v2");
