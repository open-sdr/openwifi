// Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be

const char *sdr_compatible_str = "sdr,sdr";

enum openwifi_band {
	BAND_900M = 0,
	BAND_2_4GHZ,
	BAND_3_65GHZ,
	BAND_5_0GHZ,
	BAND_5_8GHZ,
	BAND_5_9GHZ,
	BAND_60GHZ,
};

// ------------------------------------tx interface----------------------------------------
const char *tx_intf_compatible_str = "sdr,tx_intf";

#define TX_INTF_REG_MULTI_RST_ADDR                 (0*4)
#define TX_INTF_REG_MIXER_CFG_ADDR                 (1*4)
#define TX_INTF_REG_WIFI_TX_MODE_ADDR              (2*4)
#define TX_INTF_REG_IQ_SRC_SEL_ADDR                (3*4)
#define TX_INTF_REG_CTS_TOSELF_CONFIG_ADDR         (4*4)
#define TX_INTF_REG_START_TRANS_TO_PS_MODE_ADDR    (5*4)
#define TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_ADDR  (6*4)
#define TX_INTF_REG_MISC_SEL_ADDR                  (7*4)
#define TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_ADDR      (8*4)
#define TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR      (9*4)
#define TX_INTF_REG_CFG_DATA_TO_ANT_ADDR           (10*4)
#define TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL1_ADDR     (12*4)
#define TX_INTF_REG_BB_GAIN_ADDR                   (13*4)
#define TX_INTF_REG_INTERRUPT_SEL_ADDR             (14*4)
#define TX_INTF_REG_ANT_SEL_ADDR                   (16*4)
#define TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_ADDR    (21*4)
#define TX_INTF_REG_PKT_INFO_ADDR                  (22*4)
#define TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_ADDR     (24*4)

#define TX_INTF_NUM_ANTENNA                        2
#define TX_INTF_NUM_BYTE_PER_DMA_SYMBOL            (64/8)
#define TX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS    3

enum tx_intf_mode {
	TX_INTF_AXIS_LOOP_BACK = 0,
	TX_INTF_BYPASS,
	TX_INTF_BW_20MHZ_AT_0MHZ_ANT0,
	TX_INTF_BW_20MHZ_AT_0MHZ_ANT1,
	TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0,
	TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT0,
	TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1,
	TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1,
};

const int tx_intf_fo_mapping[] = {0, 0, 0, 0,-10,10,-10,10};

struct tx_intf_driver_api {
	u32 (*hw_init)(enum tx_intf_mode mode, u32 num_dma_symbol_to_pl, u32 num_dma_symbol_to_ps);

	u32 (*reg_read)(u32 reg);
	void (*reg_write)(u32 reg, u32 value);

	u32 (*TX_INTF_REG_MULTI_RST_read)(void);
	u32 (*TX_INTF_REG_MIXER_CFG_read)(void);
	u32 (*TX_INTF_REG_WIFI_TX_MODE_read)(void);
	u32 (*TX_INTF_REG_IQ_SRC_SEL_read)(void);
	u32 (*TX_INTF_REG_CTS_TOSELF_CONFIG_read)(void);
	u32 (*TX_INTF_REG_START_TRANS_TO_PS_MODE_read)(void);
	u32 (*TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_read)(void);
	u32 (*TX_INTF_REG_MISC_SEL_read)(void);
	u32 (*TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_read)(void);
	u32 (*TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read)(void);
	u32 (*TX_INTF_REG_CFG_DATA_TO_ANT_read)(void);
	u32 (*TX_INTF_REG_INTERRUPT_SEL_read)(void);
	u32 (*TX_INTF_REG_BB_GAIN_read)(void);
	u32 (*TX_INTF_REG_ANT_SEL_read)(void);
	u32 (*TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_read)(void);
	u32 (*TX_INTF_REG_PKT_INFO_read)(void);
	u32 (*TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read)(void);

	void (*TX_INTF_REG_MULTI_RST_write)(u32 value);
	void (*TX_INTF_REG_MIXER_CFG_write)(u32 value);
	void (*TX_INTF_REG_WIFI_TX_MODE_write)(u32 value);
	void (*TX_INTF_REG_IQ_SRC_SEL_write)(u32 value);
	void (*TX_INTF_REG_CTS_TOSELF_CONFIG_write)(u32 value);
	void (*TX_INTF_REG_START_TRANS_TO_PS_MODE_write)(u32 value);
	void (*TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write)(u32 value);
	void (*TX_INTF_REG_MISC_SEL_write)(u32 value);
	void (*TX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write)(u32 value);
	void (*TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write)(u32 value);
	void (*TX_INTF_REG_CFG_DATA_TO_ANT_write)(u32 value);
	void (*TX_INTF_REG_INTERRUPT_SEL_write)(u32 value);
	void (*TX_INTF_REG_BB_GAIN_write)(u32 value);
	void (*TX_INTF_REG_ANT_SEL_write)(u32 value);
	void (*TX_INTF_REG_S_AXIS_FIFO_DATA_COUNT_write)(u32 value);
	void (*TX_INTF_REG_PKT_INFO_write)(u32 value);
};

// ------------------------------------rx interface----------------------------------------
const char *rx_intf_compatible_str = "sdr,rx_intf";

#define RX_INTF_REG_MULTI_RST_ADDR                 (0*4)
#define RX_INTF_REG_MIXER_CFG_ADDR                 (1*4)
#define RX_INTF_REG_INTERRUPT_TEST_ADDR            (2*4)
#define RX_INTF_REG_IQ_SRC_SEL_ADDR                (3*4)
#define RX_INTF_REG_IQ_CTRL_ADDR                   (4*4)
#define RX_INTF_REG_START_TRANS_TO_PS_MODE_ADDR    (5*4)
#define RX_INTF_REG_START_TRANS_TO_PS_ADDR         (6*4)
#define RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_ADDR (7*4)
#define RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_ADDR      (8*4)
#define RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR      (9*4)
#define RX_INTF_REG_CFG_DATA_TO_ANT_ADDR           (10*4)
#define RX_INTF_REG_TLAST_TIMEOUT_TOP_ADDR         (12*4)
#define RX_INTF_REG_S2MM_INTR_DELAY_COUNT_ADDR     (13*4)
#define RX_INTF_REG_ANT_SEL_ADDR                   (16*4)

#define RX_INTF_NUM_ANTENNA                        2
#define RX_INTF_NUM_BYTE_PER_DMA_SYMBOL            (64/8)
#define RX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS    3

enum rx_intf_mode {
	RX_INTF_AXIS_LOOP_BACK = 0,
	RX_INTF_BYPASS,
	RX_INTF_BW_20MHZ_AT_0MHZ_ANT0,
	RX_INTF_BW_20MHZ_AT_0MHZ_ANT1,
	RX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0,
	RX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1,
	RX_INTF_BW_20MHZ_AT_P_10MHZ_ANT0,
	RX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1,
};

const int rx_intf_fo_mapping[] = {0,0,0,0,-10,-10,10,10};

struct rx_intf_driver_api {
	u32 io_start;
	u32 base_addr;
	
	u32 (*hw_init)(enum rx_intf_mode mode, u32 num_dma_symbol_to_pl, u32 num_dma_symbol_to_ps);

	u32 (*reg_read)(u32 reg);
	void (*reg_write)(u32 reg, u32 value);

	u32 (*RX_INTF_REG_MULTI_RST_read)(void);
	u32 (*RX_INTF_REG_MIXER_CFG_read)(void);
	u32 (*RX_INTF_REG_IQ_SRC_SEL_read)(void);
	u32 (*RX_INTF_REG_IQ_CTRL_read)(void);
	u32 (*RX_INTF_REG_START_TRANS_TO_PS_MODE_read)(void);
	u32 (*RX_INTF_REG_START_TRANS_TO_PS_read)(void);
	u32 (*RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_read)(void);
	u32 (*RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_read)(void);
	u32 (*RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read)(void);
	u32 (*RX_INTF_REG_CFG_DATA_TO_ANT_read)(void);
	u32 (*RX_INTF_REG_ANT_SEL_read)(void);
	u32 (*RX_INTF_REG_INTERRUPT_TEST_read)(void);
	void (*RX_INTF_REG_MULTI_RST_write)(u32 value);
	void (*RX_INTF_REG_MIXER_CFG_write)(u32 value);
	void (*RX_INTF_REG_IQ_SRC_SEL_write)(u32 value);
	void (*RX_INTF_REG_IQ_CTRL_write)(u32 value);
	void (*RX_INTF_REG_START_TRANS_TO_PS_MODE_write)(u32 value);
	void (*RX_INTF_REG_START_TRANS_TO_PS_write)(u32 value);
	void (*RX_INTF_REG_START_TRANS_TO_PS_SRC_SEL_write)(u32 value);
	void (*RX_INTF_REG_NUM_DMA_SYMBOL_TO_PL_write)(u32 value);
	void (*RX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write)(u32 value);
	void (*RX_INTF_REG_CFG_DATA_TO_ANT_write)(u32 value);
	void (*RX_INTF_REG_ANT_SEL_write)(u32 value);
	void (*RX_INTF_REG_INTERRUPT_TEST_write)(u32 value);

	void (*RX_INTF_REG_M_AXIS_RST_write)(u32 value);
	void (*RX_INTF_REG_S2MM_INTR_DELAY_COUNT_write)(u32 value);
	void (*RX_INTF_REG_TLAST_TIMEOUT_TOP_write)(u32 value);
};

// ----------------------------------openofdm rx-------------------------------
const char *openofdm_rx_compatible_str = "sdr,openofdm_rx";

#define OPENOFDM_RX_REG_MULTI_RST_ADDR     (0*4)
#define OPENOFDM_RX_REG_ENABLE_ADDR        (1*4)
#define OPENOFDM_RX_REG_POWER_THRES_ADDR   (2*4)
#define OPENOFDM_RX_REG_MIN_PLATEAU_ADDR   (3*4)
#define OPENOFDM_RX_REG_STATE_HISTORY_ADDR (20*4)

enum openofdm_rx_mode {
	OPENOFDM_RX_TEST = 0,
	OPENOFDM_RX_NORMAL,
};

struct openofdm_rx_driver_api {
	u32 power_thres;
	u32 min_plateau;

	u32 (*hw_init)(enum openofdm_rx_mode mode);

	u32 (*reg_read)(u32 reg);
	void (*reg_write)(u32 reg, u32 value);

	u32 (*OPENOFDM_RX_REG_STATE_HISTORY_read)(void);

	void (*OPENOFDM_RX_REG_MULTI_RST_write)(u32 value);
	void (*OPENOFDM_RX_REG_ENABLE_write)(u32 value);
	void (*OPENOFDM_RX_REG_POWER_THRES_write)(u32 value);
	void (*OPENOFDM_RX_REG_MIN_PLATEAU_write)(u32 value);
};

// ---------------------------------------openofdm tx-------------------------------
const char *openofdm_tx_compatible_str = "sdr,openofdm_tx";

#define OPENOFDM_TX_REG_MULTI_RST_ADDR                 (0*4)
#define OPENOFDM_TX_REG_INIT_PILOT_STATE_ADDR          (1*4)
#define OPENOFDM_TX_REG_INIT_DATA_STATE_ADDR           (2*4)

enum openofdm_tx_mode {
	OPENOFDM_TX_TEST = 0,
	OPENOFDM_TX_NORMAL,
};

struct openofdm_tx_driver_api {
	u32 (*hw_init)(enum openofdm_tx_mode mode);

	u32 (*reg_read)(u32 reg);
	void (*reg_write)(u32 reg, u32 value);

	void (*OPENOFDM_TX_REG_MULTI_RST_write)(u32 value);
	void (*OPENOFDM_TX_REG_INIT_PILOT_STATE_write)(u32 value);
	void (*OPENOFDM_TX_REG_INIT_DATA_STATE_write)(u32 value);
};

// ---------------------------------------xpu low MAC controller-------------------------------

// extra filter flag together with enum ieee80211_filter_flags in mac80211.h
#define UNICAST_FOR_US     (1<<9)
#define BROADCAST_ALL_ONE  (1<<10)
#define BROADCAST_ALL_ZERO (1<<11)
#define MY_BEACON          (1<<12)
#define MONITOR_ALL        (1<<13)

const char *xpu_compatible_str = "sdr,xpu";

#define XPU_REG_MULTI_RST_ADDR            (0*4)
#define XPU_REG_SRC_SEL_ADDR              (1*4)
#define XPU_REG_TSF_LOAD_VAL_LOW_ADDR     (2*4)
#define XPU_REG_TSF_LOAD_VAL_HIGH_ADDR    (3*4)
#define XPU_REG_BAND_CHANNEL_ADDR         (4*4)
#define XPU_REG_RSSI_DB_CFG_ADDR          (7*4)
#define XPU_REG_LBT_TH_ADDR               (8*4)
#define XPU_REG_CSMA_DEBUG_ADDR           (9*4)
#define XPU_REG_BB_RF_DELAY_ADDR          (10*4)
#define XPU_REG_MAX_NUM_RETRANS_ADDR      (11*4)
#define XPU_REG_RECV_ACK_COUNT_TOP0_ADDR  (16*4)
#define XPU_REG_RECV_ACK_COUNT_TOP1_ADDR  (17*4)
#define XPU_REG_SEND_ACK_WAIT_TOP_ADDR    (18*4)
#define XPU_REG_CSMA_CFG_ADDR             (19*4)

#define XPU_REG_SLICE_COUNT_TOTAL0_ADDR   (20*4)
#define XPU_REG_SLICE_COUNT_START0_ADDR   (21*4)
#define XPU_REG_SLICE_COUNT_END0_ADDR     (22*4)
#define XPU_REG_SLICE_COUNT_TOTAL1_ADDR   (23*4)
#define XPU_REG_SLICE_COUNT_START1_ADDR   (24*4)
#define XPU_REG_SLICE_COUNT_END1_ADDR     (25*4)

#define XPU_REG_CTS_TO_RTS_CONFIG_ADDR    (26*4)
#define XPU_REG_FILTER_FLAG_ADDR          (27*4)
#define XPU_REG_BSSID_FILTER_LOW_ADDR     (28*4)
#define XPU_REG_BSSID_FILTER_HIGH_ADDR    (29*4)
#define XPU_REG_MAC_ADDR_LOW_ADDR         (30*4)
#define XPU_REG_MAC_ADDR_HIGH_ADDR        (31*4)

#define XPU_REG_FC_DI_ADDR                (34*4)
#define XPU_REG_ADDR1_LOW_ADDR            (35*4)
#define XPU_REG_ADDR1_HIGH_ADDR           (36*4)
#define XPU_REG_ADDR2_LOW_ADDR            (37*4)
#define XPU_REG_ADDR2_HIGH_ADDR           (38*4)
#define XPU_REG_ADDR3_LOW_ADDR            (39*4)
#define XPU_REG_ADDR3_HIGH_ADDR           (40*4)

#define XPU_REG_SC_LOW_ADDR               (41*4)
#define XPU_REG_ADDR4_HIGH_ADDR           (42*4)
#define XPU_REG_ADDR4_LOW_ADDR            (43*4)

#define XPU_REG_TRX_STATUS_ADDR           (50*4)
#define XPU_REG_TX_RESULT_ADDR            (51*4)

#define XPU_REG_TSF_RUNTIME_VAL_LOW_ADDR  (58*4)
#define XPU_REG_TSF_RUNTIME_VAL_HIGH_ADDR (59*4)

#define XPU_REG_RSSI_HALF_DB_ADDR         (60*4)
#define XPU_REG_IQ_RSSI_HALF_DB_ADDR      (61*4)

enum xpu_mode {
	XPU_TEST = 0,
	XPU_NORMAL,
};

struct xpu_driver_api {
	u32 (*hw_init)(enum xpu_mode mode);

	u32 (*reg_read)(u32 reg);
	void (*reg_write)(u32 reg, u32 value);

	void (*XPU_REG_MULTI_RST_write)(u32 value);
	u32  (*XPU_REG_MULTI_RST_read)(void);

	void (*XPU_REG_SRC_SEL_write)(u32 value);
	u32  (*XPU_REG_SRC_SEL_read)(void);

	void (*XPU_REG_RECV_ACK_COUNT_TOP0_write)(u32 value);
	u32  (*XPU_REG_RECV_ACK_COUNT_TOP0_read)(void);

	void (*XPU_REG_RECV_ACK_COUNT_TOP1_write)(u32 value);
	u32  (*XPU_REG_RECV_ACK_COUNT_TOP1_read)(void);

	void (*XPU_REG_SEND_ACK_WAIT_TOP_write)(u32 value);
	u32  (*XPU_REG_SEND_ACK_WAIT_TOP_read)(void);

	void (*XPU_REG_ACK_FC_FILTER_write)(u32 value);
	u32  (*XPU_REG_ACK_FC_FILTER_read)(void);

	void (*XPU_REG_CTS_TO_RTS_CONFIG_write)(u32 value);
	u32  (*XPU_REG_CTS_TO_RTS_CONFIG_read)(void);

	void (*XPU_REG_FILTER_FLAG_write)(u32 value);
	u32  (*XPU_REG_FILTER_FLAG_read)(void);

	void (*XPU_REG_MAC_ADDR_LOW_write)(u32 value);
	u32  (*XPU_REG_MAC_ADDR_LOW_read)(void);

	void (*XPU_REG_MAC_ADDR_HIGH_write)(u32 value);
	u32  (*XPU_REG_MAC_ADDR_HIGH_read)(void);

	void (*XPU_REG_BSSID_FILTER_LOW_write)(u32 value);
	u32  (*XPU_REG_BSSID_FILTER_LOW_read)(void);

	void (*XPU_REG_BSSID_FILTER_HIGH_write)(u32 value);
	u32  (*XPU_REG_BSSID_FILTER_HIGH_read)(void);

	void (*XPU_REG_BAND_CHANNEL_write)(u32 value);
	u32  (*XPU_REG_BAND_CHANNEL_read)(void);

	u32  (*XPU_REG_TRX_STATUS_read)(void);
	u32  (*XPU_REG_TX_RESULT_read)(void);

	u32  (*XPU_REG_TSF_RUNTIME_VAL_LOW_read)(void);
	u32  (*XPU_REG_TSF_RUNTIME_VAL_HIGH_read)(void);

	void (*XPU_REG_TSF_LOAD_VAL_LOW_write)(u32 value);
	void (*XPU_REG_TSF_LOAD_VAL_HIGH_write)(u32 value);
	void (*XPU_REG_TSF_LOAD_VAL_write)(u32 high_value, u32 low_value);

	u32  (*XPU_REG_FC_DI_read)(void);
	u32  (*XPU_REG_ADDR1_LOW_read)(void);
	u32  (*XPU_REG_ADDR1_HIGH_read)(void);
	u32  (*XPU_REG_ADDR2_LOW_read)(void);
	u32  (*XPU_REG_ADDR2_HIGH_read)(void);

	void (*XPU_REG_LBT_TH_write)(u32 value);
	u32  (*XPU_REG_LBT_TH_read)(void);

	void (*XPU_REG_RSSI_DB_CFG_write)(u32 value);
	u32  (*XPU_REG_RSSI_DB_CFG_read)(void);

	void (*XPU_REG_CSMA_DEBUG_write)(u32 value);
	u32  (*XPU_REG_CSMA_DEBUG_read)(void);

	void (*XPU_REG_CSMA_CFG_write)(u32 value);
	u32  (*XPU_REG_CSMA_CFG_read)(void);

	void (*XPU_REG_SLICE_COUNT_TOTAL0_write)(u32 value);
	void (*XPU_REG_SLICE_COUNT_START0_write)(u32 value);
	void (*XPU_REG_SLICE_COUNT_END0_write)(u32 value);
	void (*XPU_REG_SLICE_COUNT_TOTAL1_write)(u32 value);
	void (*XPU_REG_SLICE_COUNT_START1_write)(u32 value);
	void (*XPU_REG_SLICE_COUNT_END1_write)(u32 value);

	u32 (*XPU_REG_SLICE_COUNT_TOTAL0_read)(void);
	u32 (*XPU_REG_SLICE_COUNT_START0_read)(void);
	u32 (*XPU_REG_SLICE_COUNT_END0_read)(void);
	u32 (*XPU_REG_SLICE_COUNT_TOTAL1_read)(void);
	u32 (*XPU_REG_SLICE_COUNT_START1_read)(void);
	u32 (*XPU_REG_SLICE_COUNT_END1_read)(void);

	void (*XPU_REG_BB_RF_DELAY_write)(u32 value);
	void (*XPU_REG_MAX_NUM_RETRANS_write)(u32 value);

	void (*XPU_REG_MAC_ADDR_write)(u8 *mac_addr);
};
