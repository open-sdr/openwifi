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

static inline u32 XPU_REG_TRX_STATUS_read(void){
	return reg_read(XPU_REG_TRX_STATUS_ADDR);
}

static inline u32 XPU_REG_TX_RESULT_read(void){
	return reg_read(XPU_REG_TX_RESULT_ADDR);
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

static inline u32 XPU_REG_FC_DI_read(void){
	return reg_read(XPU_REG_FC_DI_ADDR);
}

static inline u32 XPU_REG_ADDR1_LOW_read(void){
	return reg_read(XPU_REG_ADDR1_LOW_ADDR);
}

static inline u32 XPU_REG_ADDR1_HIGH_read(void){
	return reg_read(XPU_REG_ADDR1_HIGH_ADDR);
}

static inline u32 XPU_REG_ADDR2_LOW_read(void){
	return reg_read(XPU_REG_ADDR2_LOW_ADDR);
}

static inline u32 XPU_REG_ADDR2_HIGH_read(void){
	return reg_read(XPU_REG_ADDR2_HIGH_ADDR);
}

// static inline void XPU_REG_LBT_TH_write(u32 value, u32 en_flag) {
// 	if (en_flag) {
// 		reg_write(XPU_REG_LBT_TH_ADDR, value&0x7FFFFFFF);
// 	} else {
// 		reg_write(XPU_REG_LBT_TH_ADDR, value|0x80000000);
// 	}
// }

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

static inline void XPU_REG_SLICE_COUNT_TOTAL0_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_TOTAL0_ADDR, value);
}
static inline void XPU_REG_SLICE_COUNT_START0_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_START0_ADDR, value);
}
static inline void XPU_REG_SLICE_COUNT_END0_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_END0_ADDR, value);
}
static inline void XPU_REG_SLICE_COUNT_TOTAL1_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_TOTAL1_ADDR, value);
}
static inline void XPU_REG_SLICE_COUNT_START1_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_START1_ADDR, value);
}
static inline void XPU_REG_SLICE_COUNT_END1_write(u32 value){
	reg_write(XPU_REG_SLICE_COUNT_END1_ADDR, value);
}

static inline u32 XPU_REG_SLICE_COUNT_TOTAL0_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_TOTAL0_ADDR);
}
static inline u32 XPU_REG_SLICE_COUNT_START0_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_START0_ADDR);
}
static inline u32 XPU_REG_SLICE_COUNT_END0_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_END0_ADDR);
}
static inline u32 XPU_REG_SLICE_COUNT_TOTAL1_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_TOTAL1_ADDR);
}
static inline u32 XPU_REG_SLICE_COUNT_START1_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_START1_ADDR);
}
static inline u32 XPU_REG_SLICE_COUNT_END1_read(void){
	return reg_read(XPU_REG_SLICE_COUNT_END1_ADDR);
}

static inline void XPU_REG_BB_RF_DELAY_write(u32 value){
	reg_write(XPU_REG_BB_RF_DELAY_ADDR, value);
}

static inline void XPU_REG_MAX_NUM_RETRANS_write(u32 value){
	reg_write(XPU_REG_MAX_NUM_RETRANS_ADDR, value);
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
static struct xpu_driver_api *xpu_api = &xpu_driver_api_inst;
EXPORT_SYMBOL(xpu_api);

static inline u32 hw_init(enum xpu_mode mode){
	int err=0, rssi_half_db_th, rssi_half_db_offset, agc_gain_delay;
	u32 reg_val;
	u32 filter_flag = 0;

	printk("%s hw_init mode %d\n", xpu_compatible_str, mode);

	//rst internal module
	for (reg_val=0;reg_val<32;reg_val++)
		xpu_api->XPU_REG_MULTI_RST_write(0xFFFFFFFF);
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
	filter_flag = (FIF_ALLMULTI|FIF_FCSFAIL|FIF_PLCPFAIL|FIF_BCN_PRBRESP_PROMISC|FIF_CONTROL|FIF_OTHER_BSS|FIF_PSPOLL|FIF_PROBE_REQ|UNICAST_FOR_US|BROADCAST_ALL_ONE|BROADCAST_ALL_ZERO|MY_BEACON|MONITOR_ALL);
	xpu_api->XPU_REG_FILTER_FLAG_write(filter_flag);
	xpu_api->XPU_REG_CTS_TO_RTS_CONFIG_write(0xB<<16);//6M 1011:0xB

	////set up FC type filter for packet needs ACK -- no use, FPGA handle by itself
	//xpu_api->XPU_REG_ACK_FC_FILTER_write((3<<(2+16))|(2<<2)); // low 16 bits target FC 16 bits; high 16 bits -- mask

	// after send data frame wait for ACK, this will be set in real time in function ad9361_rf_set_channel
	// xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write( (((51+2)*200)<<16) | 200 ); // high 16 bits to cover sig valid of ACK packet, low 16 bits is adjustment of fcs valid waiting time.  let's add 2us for those device that is really "slow"!
	// xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( 1200 ); // +6 = 16us for 5GHz

	//xpu_api->XPU_REG_MAX_NUM_RETRANS_write(3); // if this > 0, it will override mac80211 set value, and set static retransmission limit
	
	xpu_api->XPU_REG_BB_RF_DELAY_write(975);

	xpu_api->XPU_REG_SLICE_COUNT_TOTAL0_write(50000-1); // total 50ms
	xpu_api->XPU_REG_SLICE_COUNT_START0_write(0); //start 0ms
	xpu_api->XPU_REG_SLICE_COUNT_END0_write(50000-1); //end 10ms
	xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_write(50000-1); // total 50ms
	xpu_api->XPU_REG_SLICE_COUNT_START1_write(0000); //start 0ms
	xpu_api->XPU_REG_SLICE_COUNT_END1_write(1000-1); //end 1ms

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
	xpu_api->XPU_REG_BAND_CHANNEL_write((false<<24)|(BAND_5_8GHZ<<16)|44);//use_short_slot==false; 5.8GHz; channel 44 -- default setting to sync with priv->band/channel/use_short_slot

	agc_gain_delay = 50; //samples
	rssi_half_db_offset = 75<<1;
	xpu_api->XPU_REG_RSSI_DB_CFG_write(0x80000000|((rssi_half_db_offset<<16)|agc_gain_delay) );
	xpu_api->XPU_REG_RSSI_DB_CFG_write((~0x80000000)&((rssi_half_db_offset<<16)|agc_gain_delay) );
	
	//rssi_half_db_th = 70<<1; // with splitter
	rssi_half_db_th = 87<<1; // -62dBm
	xpu_api->XPU_REG_LBT_TH_write(rssi_half_db_th); // set IQ rssi th step .5dB to xxx and enable it

	//xpu_api->XPU_REG_CSMA_DEBUG_write((1<<31)|(20<<24)|(4<<19)|(3<<14)|(10<<7)|(5));
	xpu_api->XPU_REG_CSMA_DEBUG_write(0);
	
	//xpu_api->XPU_REG_CSMA_CFG_write(3); //normal CSMA
	xpu_api->XPU_REG_CSMA_CFG_write(0xe0000000); //high priority

	// xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( ((1030-238)<<16)|0 );//high 16bit 5GHz; low 16 bit 2.4GHz (Attention, current tx core has around 1.19us starting delay that makes the ack fall behind 10us SIFS in 2.4GHz! Need to improve TX in 2.4GHz!)
	xpu_api->XPU_REG_SEND_ACK_WAIT_TOP_write( ((1030)<<16)|0 );//now our tx send out I/Q immediately

	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP0_write( (((45+2+2)*200 + 300)<<16) | 200 );//2.4GHz. extra 300 clocks are needed when rx core fall into fake ht detection phase (rx mcs 6M)
	xpu_api->XPU_REG_RECV_ACK_COUNT_TOP1_write( (((51+2+2)*200 + 300)<<16) | 200 );//5GHz. extra 300 clocks are needed when rx core fall into fake ht detection phase (rx mcs 6M)

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

	xpu_api->XPU_REG_TRX_STATUS_read=XPU_REG_TRX_STATUS_read;
	xpu_api->XPU_REG_TX_RESULT_read=XPU_REG_TX_RESULT_read;

	xpu_api->XPU_REG_TSF_RUNTIME_VAL_LOW_read=XPU_REG_TSF_RUNTIME_VAL_LOW_read;
	xpu_api->XPU_REG_TSF_RUNTIME_VAL_HIGH_read=XPU_REG_TSF_RUNTIME_VAL_HIGH_read;
	xpu_api->XPU_REG_TSF_LOAD_VAL_LOW_write=XPU_REG_TSF_LOAD_VAL_LOW_write;
	xpu_api->XPU_REG_TSF_LOAD_VAL_HIGH_write=XPU_REG_TSF_LOAD_VAL_HIGH_write;
	xpu_api->XPU_REG_TSF_LOAD_VAL_write=XPU_REG_TSF_LOAD_VAL_write;
	
	xpu_api->XPU_REG_FC_DI_read=XPU_REG_FC_DI_read;
	xpu_api->XPU_REG_ADDR1_LOW_read=XPU_REG_ADDR1_LOW_read;
	xpu_api->XPU_REG_ADDR1_HIGH_read=XPU_REG_ADDR1_HIGH_read;
	xpu_api->XPU_REG_ADDR2_LOW_read=XPU_REG_ADDR2_LOW_read;
	xpu_api->XPU_REG_ADDR2_HIGH_read=XPU_REG_ADDR2_HIGH_read;

	xpu_api->XPU_REG_LBT_TH_write=XPU_REG_LBT_TH_write;
	xpu_api->XPU_REG_LBT_TH_read=XPU_REG_LBT_TH_read;

	xpu_api->XPU_REG_RSSI_DB_CFG_read=XPU_REG_RSSI_DB_CFG_read;
	xpu_api->XPU_REG_RSSI_DB_CFG_write=XPU_REG_RSSI_DB_CFG_write;

	xpu_api->XPU_REG_CSMA_DEBUG_write=XPU_REG_CSMA_DEBUG_write;
	xpu_api->XPU_REG_CSMA_DEBUG_read=XPU_REG_CSMA_DEBUG_read;

	xpu_api->XPU_REG_CSMA_CFG_write=XPU_REG_CSMA_CFG_write;
	xpu_api->XPU_REG_CSMA_CFG_read=XPU_REG_CSMA_CFG_read;

	xpu_api->XPU_REG_SLICE_COUNT_TOTAL0_write=XPU_REG_SLICE_COUNT_TOTAL0_write;
	xpu_api->XPU_REG_SLICE_COUNT_START0_write=XPU_REG_SLICE_COUNT_START0_write;
	xpu_api->XPU_REG_SLICE_COUNT_END0_write=XPU_REG_SLICE_COUNT_END0_write;
	xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_write=XPU_REG_SLICE_COUNT_TOTAL1_write;
	xpu_api->XPU_REG_SLICE_COUNT_START1_write=XPU_REG_SLICE_COUNT_START1_write;
	xpu_api->XPU_REG_SLICE_COUNT_END1_write=XPU_REG_SLICE_COUNT_END1_write;

	xpu_api->XPU_REG_SLICE_COUNT_TOTAL0_read=XPU_REG_SLICE_COUNT_TOTAL0_read;
	xpu_api->XPU_REG_SLICE_COUNT_START0_read=XPU_REG_SLICE_COUNT_START0_read;
	xpu_api->XPU_REG_SLICE_COUNT_END0_read=XPU_REG_SLICE_COUNT_END0_read;
	xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_read=XPU_REG_SLICE_COUNT_TOTAL1_read;
	xpu_api->XPU_REG_SLICE_COUNT_START1_read=XPU_REG_SLICE_COUNT_START1_read;
	xpu_api->XPU_REG_SLICE_COUNT_END1_read=XPU_REG_SLICE_COUNT_END1_read;

	xpu_api->XPU_REG_BB_RF_DELAY_write=XPU_REG_BB_RF_DELAY_write;
	xpu_api->XPU_REG_MAX_NUM_RETRANS_write=XPU_REG_MAX_NUM_RETRANS_write;

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
