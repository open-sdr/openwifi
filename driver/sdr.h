// Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be

#ifndef OPENWIFI_SDR
#define OPENWIFI_SDR

// -------------------for leds--------------------------------
struct gpio_led_data { //pleas always align with the leds-gpio.c in linux kernel
	struct led_classdev cdev;
	struct gpio_desc *gpiod;
	u8 can_sleep;
	u8 blinking;
	gpio_blink_set_t platform_gpio_blink_set;
};

struct gpio_leds_priv { //pleas always align with the leds-gpio.c in linux kernel
	int num_leds;
	struct gpio_led_data leds[];
};

struct openwifi_rf_ops {
	char *name;
//	void (*init)(struct ieee80211_hw *);
//	void (*stop)(struct ieee80211_hw *);
	void (*set_chan)(struct ieee80211_hw *, struct ieee80211_conf *);
//	u8 (*calc_rssi)(u8 agc, u8 sq);
};

struct openwifi_buffer_descriptor {
	u32 num_dma_byte;
    u32 sn;
    u32 hw_queue_idx;
    u32 retry_limit;
    u32 need_ack;
    struct sk_buff *skb_linked;
    dma_addr_t dma_mapping_addr;
    u32 reserved;
} __packed;

struct openwifi_ring {
	struct openwifi_buffer_descriptor *bds;
    u32 bd_wr_idx;
	u32 bd_rd_idx;
    u32 reserved;
} __packed;

struct openwifi_vif {
	struct ieee80211_hw *dev;

	int idx; // this vif's idx on the dev

	/* beaconing */
	struct delayed_work beacon_work;
	bool enable_beacon;
};

union u32_byte4 {
	u32 a;
	u8 c[4];
};
union u16_byte2 {
	u16 a;
	u8 c[2];
};

#define MAX_NUM_DRV_REG 32
#define MAX_NUM_LED 4
#define OPENWIFI_LED_MAX_NAME_LEN 32

#define MAX_NUM_VIF 4

#define LEN_PHY_HEADER 16
#define LEN_PHY_CRC 4

#define NUM_TX_BD 32
#define NUM_RX_BD 16
#define TX_BD_BUF_SIZE (8192)
#define RX_BD_BUF_SIZE (8192)

#define NUM_BIT_MAX_NUM_HW_QUEUE 2
#define MAX_NUM_HW_QUEUE 2
#define NUM_BIT_MAX_PHY_TX_SN 12
#define MAX_PHY_TX_SN ((1<<NUM_BIT_MAX_PHY_TX_SN)-1)

#define AD9361_RADIO_OFF_TX_ATT 89750 //please align with ad9361.c
#define AD9361_RADIO_ON_TX_ATT 000    //please align with rf_init.sh

#define SDR_SUPPORTED_FILTERS	\
	(FIF_ALLMULTI |				\
	FIF_BCN_PRBRESP_PROMISC |	\
	FIF_CONTROL |				\
	FIF_OTHER_BSS |				\
	FIF_PSPOLL |				\
	FIF_PROBE_REQ)

#define HIGH_PRIORITY_DISCARD_FLAG ((~0x040)<<16) // don't force drop OTHER_BSS by high priority discard
//#define HIGH_PRIORITY_DISCARD_FLAG ((~0x140)<<16) // don't force drop OTHER_BSS and PROB_REQ by high priority discard

/* 5G chan 36 - chan 64*/
#define SDR_5GHZ_CH36_64	\
	REG_RULE(5150-10, 5350+10, 80, 0, 20, 0)
/* 5G chan 36 - chan 48*/
#define SDR_5GHZ_CH36_48	\
	REG_RULE(5150-10, 5270+10, 80, 0, 20, 0)

/*
 *Only these channels all allow active
 *scan on all world regulatory domains
 */
#define SDR_2GHZ_CH01_13	REG_RULE(2412-10, 2472+10, 40, 0, 20, NL80211_RRF_NO_CCK) // disable 11b
#define SDR_2GHZ_CH01_14	REG_RULE(2412-10, 2484+10, 40, 0, 20, NL80211_RRF_NO_CCK) // disable 11b

// regulatory.h alpha2
//  *	00 - World regulatory domain
//  *	99 - built by driver but a specific alpha2 cannot be determined
//  *	98 - result of an intersection between two regulatory domains
//  *	97 - regulatory domain has not yet been configured
static const struct ieee80211_regdomain sdr_regd = { // for wiphy_apply_custom_regulatory
	.n_reg_rules = 2,
	.alpha2 = "99",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		//SDR_2GHZ_CH01_13,
		//SDR_5GHZ_CH36_48, //Avoid radar!
		SDR_2GHZ_CH01_14,
		SDR_5GHZ_CH36_64,
		}
};

#define CHAN2G(_channel, _freq, _flags) { \
	.band			= NL80211_BAND_2GHZ, \
	.hw_value		= (_channel), \
	.center_freq		= (_freq), \
	.flags			= (_flags), \
	.max_antenna_gain	= 0, \
	.max_power		= 0, \
}

#define CHAN5G(_channel, _freq, _flags) { \
	.band			= NL80211_BAND_5GHZ, \
	.hw_value		= (_channel), \
	.center_freq		= (_freq), \
	.flags			= (_flags), \
	.max_antenna_gain	= 0, \
	.max_power		= 0, \
}

static const struct ieee80211_rate openwifi_5GHz_rates[] = {
	{ .bitrate = 10,  .hw_value = 0,  .flags = 0},
	{ .bitrate = 20,  .hw_value = 1,  .flags = 0},
	{ .bitrate = 55,  .hw_value = 2,  .flags = 0},
	{ .bitrate = 110, .hw_value = 3,  .flags = 0},
	{ .bitrate = 60,  .hw_value = 4,  .flags = IEEE80211_RATE_MANDATORY_A},
	{ .bitrate = 90,  .hw_value = 5,  .flags = IEEE80211_RATE_MANDATORY_A},
	{ .bitrate = 120, .hw_value = 6,  .flags = IEEE80211_RATE_MANDATORY_A},
	{ .bitrate = 180, .hw_value = 7,  .flags = IEEE80211_RATE_MANDATORY_A},
	{ .bitrate = 240, .hw_value = 8,  .flags = IEEE80211_RATE_MANDATORY_A},
	{ .bitrate = 360, .hw_value = 9,  .flags = IEEE80211_RATE_MANDATORY_A},
	{ .bitrate = 480, .hw_value = 10, .flags = IEEE80211_RATE_MANDATORY_A},
	{ .bitrate = 540, .hw_value = 11, .flags = IEEE80211_RATE_MANDATORY_A},
};

static const struct ieee80211_rate openwifi_2GHz_rates[] = {
	{ .bitrate = 10,  .hw_value = 0,  .flags = 0},
	{ .bitrate = 20,  .hw_value = 1,  .flags = 0},
	{ .bitrate = 55,  .hw_value = 2,  .flags = 0},
	{ .bitrate = 110, .hw_value = 3,  .flags = 0},
	{ .bitrate = 60,  .hw_value = 4,  .flags = IEEE80211_RATE_MANDATORY_G|IEEE80211_RATE_ERP_G},
	{ .bitrate = 90,  .hw_value = 5,  .flags = IEEE80211_RATE_MANDATORY_G|IEEE80211_RATE_ERP_G},
	{ .bitrate = 120, .hw_value = 6,  .flags = IEEE80211_RATE_MANDATORY_G|IEEE80211_RATE_ERP_G},
	{ .bitrate = 180, .hw_value = 7,  .flags = IEEE80211_RATE_MANDATORY_G|IEEE80211_RATE_ERP_G},
	{ .bitrate = 240, .hw_value = 8,  .flags = IEEE80211_RATE_MANDATORY_G|IEEE80211_RATE_ERP_G},
	{ .bitrate = 360, .hw_value = 9,  .flags = IEEE80211_RATE_MANDATORY_G|IEEE80211_RATE_ERP_G},
	{ .bitrate = 480, .hw_value = 10, .flags = IEEE80211_RATE_MANDATORY_G|IEEE80211_RATE_ERP_G},
	{ .bitrate = 540, .hw_value = 11, .flags = IEEE80211_RATE_MANDATORY_G|IEEE80211_RATE_ERP_G},
};

static const struct ieee80211_channel openwifi_2GHz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

static const struct ieee80211_channel openwifi_5GHz_channels[] = {
	CHAN5G(36, 5180, 0),
	CHAN5G(38, 5190, 0),
	CHAN5G(40, 5200, 0),
	CHAN5G(42, 5210, 0),
	CHAN5G(44, 5220, 0),
	CHAN5G(46, 5230, 0),
	CHAN5G(48, 5240, 0),
	CHAN5G(52, 5260, IEEE80211_CHAN_RADAR),
	CHAN5G(56, 5280, IEEE80211_CHAN_RADAR),
	CHAN5G(60, 5300, IEEE80211_CHAN_RADAR),
	CHAN5G(64, 5320, IEEE80211_CHAN_RADAR),
	// CHAN5G(100, 5500, 0),
	// CHAN5G(104, 5520, 0),
	// CHAN5G(108, 5540, 0),
	// CHAN5G(112, 5560, 0),
	// CHAN5G(116, 5580, 0),
	// CHAN5G(120, 5600, 0),
	// CHAN5G(124, 5620, 0),
	// CHAN5G(128, 5640, 0),
	// CHAN5G(132, 5660, 0),
	// CHAN5G(136, 5680, 0),
	// CHAN5G(140, 5700, 0),
	// CHAN5G(144, 5720, 0),
	// CHAN5G(149, 5745, 0),
	// CHAN5G(153, 5765, 0),
	// CHAN5G(157, 5785, 0),
	// CHAN5G(161, 5805, 0),
	// CHAN5G(165, 5825, 0),
	// CHAN5G(169, 5845, 0),
};

static const struct ieee80211_iface_limit openwifi_if_limits[] = {
	{ .max = 2048,	.types = BIT(NL80211_IFTYPE_STATION) },
	{ .max = 4,	.types =
#ifdef CONFIG_MAC80211_MESH
				 BIT(NL80211_IFTYPE_MESH_POINT) |
#endif
				 BIT(NL80211_IFTYPE_AP) },
};

static const struct ieee80211_iface_combination openwifi_if_comb = {
	.limits = openwifi_if_limits,
	.n_limits = ARRAY_SIZE(openwifi_if_limits),
	.max_interfaces = 2048,
	.num_different_channels = 1,
	.radar_detect_widths =	BIT(NL80211_CHAN_WIDTH_20_NOHT) |
					BIT(NL80211_CHAN_WIDTH_20) |
					BIT(NL80211_CHAN_WIDTH_40) |
					BIT(NL80211_CHAN_WIDTH_80),
};

static const u8  wifi_rate_table_mapping[16] =     { 0,  0,  0,  0,  0,  0,  0,  0, 10,   8,   6,   4, 11,  9,  7, 5};
static const u8  wifi_rate_table[16] =             { 0,  0,  0,  0,  0,  0,  0,  0, 48,  24,  12,   6, 54, 36, 18, 9};
static const u8  wifi_rate_all[16] =               { 1,  2,  5, 11,  6,  9, 12, 18, 24,  36,  48,  54,  0,  0,  0, 0};
static const u8  wifi_mcs_table_11b_force_up[16] = {11, 11, 11, 11, 11, 15, 10, 14,  9,  13,   8,  12,  0,  0,  0, 0};
static const u16 wifi_n_dbps_table[16] =           {24, 24, 24, 24, 24, 36, 48, 72, 96, 144, 192, 216,  0,  0,  0, 0};
// static const u8 wifi_mcs_table[8] =             {6,9,12,18,24,36,48,54};
// static const u8 wifi_mcs_table_phy_tx[8]    =   {11,15,10,14,9,13,8,12};

#define RX_DMA_CYCLIC_MODE
struct openwifi_priv {
	struct platform_device *pdev;
	struct ieee80211_vif *vif[MAX_NUM_VIF];

	const struct openwifi_rf_ops *rf;

	struct cf_axi_dds_state *dds_st;  //axi_ad9361 hdl ref design module, dac channel
	struct axiadc_state *adc_st;      //axi_ad9361 hdl ref design module, adc channel
	struct ad9361_rf_phy *ad9361_phy; //ad9361 chip
	struct ctrl_outs_control ctrl_out;

	int rx_freq_offset_to_lo_MHz;
	int tx_freq_offset_to_lo_MHz;
	u32 rf_bw;
	u32 actual_rx_lo;

	struct ieee80211_rate rates_2GHz[12];
	struct ieee80211_rate rates_5GHz[12];
	struct ieee80211_channel channels_2GHz[14];
	struct ieee80211_channel channels_5GHz[11];
	struct ieee80211_supported_band band_2GHz;
	struct ieee80211_supported_band band_5GHz;
	bool rfkill_off;

	int rssi_correction; // dynamic RSSI correction according to current channel in _rf_set_channel()
	
	enum rx_intf_mode rx_intf_cfg;
	enum tx_intf_mode tx_intf_cfg;
	enum openofdm_rx_mode openofdm_rx_cfg;
	enum openofdm_tx_mode openofdm_tx_cfg;
	enum xpu_mode xpu_cfg;

	int irq_rx;
	int irq_tx;

	u8 *rx_cyclic_buf;
	dma_addr_t rx_cyclic_buf_dma_mapping_addr;
	struct dma_chan *rx_chan;
	struct dma_async_tx_descriptor *rxd;
	dma_cookie_t rx_cookie;

	struct openwifi_ring tx_ring;
	struct scatterlist tx_sg;
	struct dma_chan *tx_chan;
	struct dma_async_tx_descriptor *txd;
	dma_cookie_t tx_cookie;
	bool tx_queue_stopped;

	int phy_tx_sn;
	u32 dest_mac_addr_queue_map[MAX_NUM_HW_QUEUE];
	u8 mac_addr[ETH_ALEN];
	u16 seqno;

	bool use_short_slot;
	u8 band;
	u16 channel;

	u32 drv_rx_reg_val[MAX_NUM_DRV_REG];
	u32 drv_tx_reg_val[MAX_NUM_DRV_REG];
	u32 drv_xpu_reg_val[MAX_NUM_DRV_REG];
	// u8 num_led;
	// struct led_classdev *led[MAX_NUM_LED];//zc706 has 4 user leds. please find openwifi_dev_probe to see how we get them.
	// char led_name[MAX_NUM_LED][OPENWIFI_LED_MAX_NAME_LEN];

	spinlock_t lock;
};

#endif /* OPENWIFI_SDR */
