// Author: Xianjun jiao, Michael Mehari, Wei Liu
// SPDX-FileCopyrightText: 2019 UGent
// SPDX-License-Identifier: AGPL-3.0-or-later

// #ifndef __HW_DEF_H_FILE__
// #define __HW_DEF_H_FILE__
const char *sdr_compatible_str = "sdr,sdr";

enum openwifi_hardware_type {
  ZYNQ_AD9361 = 0,
  ZYNQMP_AD9361 = 1,
  RFSOC4X2 = 2,
  UNKNOWN_HARDWARE,
};

enum openwifi_fpga_type {
  SMALL_FPGA = 0,
  LARGE_FPGA = 1,
};

//we choose 3822=(5160+2484)/2 for calibration to avoid treating 5140 as 2.4GHz
#define OPENWIFI_FREQ_MHz_TH_FOR_2_4GHZ_5GHZ 3822

enum openwifi_band {
  BAND_900M = 0,
  BAND_2_4GHZ,
  BAND_3_65GHZ,
  BAND_5_0GHZ,
  //use this BAND_5_8GHZ to represent all frequencies above OPENWIFI_FREQ_TH_FOR_2_4GHZ_5GHZ
  BAND_5_8GHZ,
  BAND_5_9GHZ,
  BAND_60GHZ,
};

// ------------------------------------tx interface----------------------------------------
const char *tx_intf_compatible_str = "sdr,tx_intf";

#define TX_INTF_REG_MULTI_RST_ADDR                 (0*4)
#define TX_INTF_REG_ARBITRARY_IQ_ADDR              (1*4)
#define TX_INTF_REG_WIFI_TX_MODE_ADDR              (2*4)
#define TX_INTF_REG_CTS_TOSELF_CONFIG_ADDR         (4*4)
#define TX_INTF_REG_CSI_FUZZER_ADDR                (5*4)
#define TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_ADDR  (6*4)
#define TX_INTF_REG_ARBITRARY_IQ_CTL_ADDR          (7*4)
#define TX_INTF_REG_TX_CONFIG_ADDR                 (8*4)
#define TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_ADDR      (9*4)
#define TX_INTF_REG_CFG_DATA_TO_ANT_ADDR           (10*4)
#define TX_INTF_REG_S_AXIS_FIFO_TH_ADDR            (11*4)
#define TX_INTF_REG_TX_HOLD_THRESHOLD_ADDR         (12*4)
#define TX_INTF_REG_BB_GAIN_ADDR                   (13*4)
#define TX_INTF_REG_INTERRUPT_SEL_ADDR             (14*4)
#define TX_INTF_REG_AMPDU_ACTION_CONFIG_ADDR       (15*4)
#define TX_INTF_REG_ANT_SEL_ADDR                   (16*4)
#define TX_INTF_REG_PHY_HDR_CONFIG_ADDR            (17*4)
#define TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_ADDR       (21*4)
#define TX_INTF_REG_PKT_INFO1_ADDR                 (22*4)
#define TX_INTF_REG_PKT_INFO2_ADDR                 (23*4)
#define TX_INTF_REG_PKT_INFO3_ADDR                 (24*4)
#define TX_INTF_REG_PKT_INFO4_ADDR                 (25*4)
#define TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_ADDR     (26*4)

#define TX_INTF_NUM_ANTENNA                        2
#define TX_INTF_NUM_BYTE_PER_DMA_SYMBOL            (64/8)
#define TX_INTF_NUM_BYTE_PER_DMA_SYMBOL_IN_BITS    3

enum tx_intf_mode {
  TX_INTF_AXIS_LOOP_BACK = 0,
  TX_INTF_BYPASS,
  TX_INTF_BW_20MHZ_AT_0MHZ_ANT0,
  TX_INTF_BW_20MHZ_AT_0MHZ_ANT1,
  TX_INTF_BW_20MHZ_AT_0MHZ_ANT_BOTH,
  TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT0,
  TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT0,
  TX_INTF_BW_20MHZ_AT_N_10MHZ_ANT1,
  TX_INTF_BW_20MHZ_AT_P_10MHZ_ANT1,
};

const int tx_intf_fo_mapping[] = {0, 0, 0, 0, 0, -10, 10, -10, 10};
const u32 dma_symbol_fifo_size_hw_queue[] = {4*1024, 4*1024, 4*1024, 4*1024}; // !!!make sure align to fifo in tx_intf_s_axis.v

struct tx_intf_driver_api {
  u32 (*hw_init)(enum tx_intf_mode mode, u32 tx_config, u32 num_dma_symbol_to_ps, enum openwifi_fpga_type fpga_type);

  u32 (*reg_read)(u32 reg);
  void (*reg_write)(u32 reg, u32 value);

  u32 (*TX_INTF_REG_MULTI_RST_read)(void);
  u32 (*TX_INTF_REG_ARBITRARY_IQ_read)(void);
  u32 (*TX_INTF_REG_WIFI_TX_MODE_read)(void);
  u32 (*TX_INTF_REG_CTS_TOSELF_CONFIG_read)(void);
  u32 (*TX_INTF_REG_CSI_FUZZER_read)(void);
  u32 (*TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_read)(void);
  u32 (*TX_INTF_REG_ARBITRARY_IQ_CTL_read)(void);
  u32 (*TX_INTF_REG_TX_CONFIG_read)(void);
  u32 (*TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_read)(void);
  u32 (*TX_INTF_REG_CFG_DATA_TO_ANT_read)(void);
  u32 (*TX_INTF_REG_S_AXIS_FIFO_TH_read)(void);
  u32 (*TX_INTF_REG_TX_HOLD_THRESHOLD_read)(void);
  u32 (*TX_INTF_REG_INTERRUPT_SEL_read)(void);
  u32 (*TX_INTF_REG_AMPDU_ACTION_CONFIG_read)(void);
  u32 (*TX_INTF_REG_BB_GAIN_read)(void);
  u32 (*TX_INTF_REG_ANT_SEL_read)(void);
  u32 (*TX_INTF_REG_PHY_HDR_CONFIG_read)(void);
  u32 (*TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_read)(void);
  u32 (*TX_INTF_REG_PKT_INFO1_read)(void);
  u32 (*TX_INTF_REG_PKT_INFO2_read)(void);
  u32 (*TX_INTF_REG_PKT_INFO3_read)(void);
  u32 (*TX_INTF_REG_PKT_INFO4_read)(void);
  u32 (*TX_INTF_REG_QUEUE_FIFO_DATA_COUNT_read)(void);

  void (*TX_INTF_REG_MULTI_RST_write)(u32 value);
  void (*TX_INTF_REG_ARBITRARY_IQ_write)(u32 value);
  void (*TX_INTF_REG_WIFI_TX_MODE_write)(u32 value);
  void (*TX_INTF_REG_CTS_TOSELF_CONFIG_write)(u32 value);
  void (*TX_INTF_REG_CSI_FUZZER_write)(u32 value);
  void (*TX_INTF_REG_CTS_TOSELF_WAIT_SIFS_TOP_write)(u32 value);
  void (*TX_INTF_REG_ARBITRARY_IQ_CTL_write)(u32 value);
  void (*TX_INTF_REG_TX_CONFIG_write)(u32 value);
  void (*TX_INTF_REG_NUM_DMA_SYMBOL_TO_PS_write)(u32 value);
  void (*TX_INTF_REG_CFG_DATA_TO_ANT_write)(u32 value);
  void (*TX_INTF_REG_S_AXIS_FIFO_TH_write)(u32 value);
  void (*TX_INTF_REG_TX_HOLD_THRESHOLD_write)(u32 value);
  void (*TX_INTF_REG_INTERRUPT_SEL_write)(u32 value);
  void (*TX_INTF_REG_AMPDU_ACTION_CONFIG_write)(u32 value);
  void (*TX_INTF_REG_BB_GAIN_write)(u32 value);
  void (*TX_INTF_REG_ANT_SEL_write)(u32 value);
  void (*TX_INTF_REG_PHY_HDR_CONFIG_write)(u32 value);
  void (*TX_INTF_REG_S_AXIS_FIFO_NO_ROOM_write)(u32 value);
  void (*TX_INTF_REG_PKT_INFO1_write)(u32 value);
  void (*TX_INTF_REG_PKT_INFO2_write)(u32 value);
  void (*TX_INTF_REG_PKT_INFO3_write)(u32 value);
  void (*TX_INTF_REG_PKT_INFO4_write)(u32 value);
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
#define RX_INTF_REG_BB_GAIN_ADDR                   (11*4)
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
  void (*RX_INTF_REG_BB_GAIN_write)(u32 value);
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
#define OPENOFDM_RX_REG_SOFT_DECODING_ADDR (4*4)
#define OPENOFDM_RX_REG_FFT_WIN_SHIFT_ADDR (5*4)
#define OPENOFDM_RX_REG_PHASE_OFFSET_ABS_TH_ADDR (18*4)
#define OPENOFDM_RX_REG_STATE_HISTORY_ADDR (20*4)

enum openofdm_rx_mode {
  OPENOFDM_RX_TEST = 0,
  OPENOFDM_RX_NORMAL,
};

#define OPENOFDM_RX_POWER_THRES_INIT 124
// Above 118 is based on these test result (2022-03-09)
// FMCOMMS3
// 2437M
// 11a/g BPSK 6M, Rx sensitivity level dmesg report -85dBm
// priv->rssi_correction = 153; rssi_half_db/2 = 153-85=68; rssi_half_db = 136
// 5180M
// 11a/g BPSK 6m, Rx sensitivity level dmesg report -84dBm
// priv->rssi_correction = 145; rssi_half_db/2 = 145-84=61; rssi_half_db = 122
// 5320M
// 11a/g BPSK 6m, Rx sensitivity level dmesg report -86dBm
// priv->rssi_correction = 148; rssi_half_db/2 = 148-86=62; rssi_half_db = 124

// FMCOMMS2
// 2437M
// 11a/g BPSK 6M, Rx sensitivity level dmesg report -80dBm
// priv->rssi_correction = 153; rssi_half_db/2 = 153-80=73; rssi_half_db = 146
// 5180M
// 11a/g BPSK 6m, Rx sensitivity level dmesg report -83dBm
// priv->rssi_correction = 145; rssi_half_db/2 = 145-83=62; rssi_half_db = 124
// 5320M
// 11a/g BPSK 6m, Rx sensitivity level dmesg report -86dBm
// priv->rssi_correction = 148; rssi_half_db/2 = 148-86=62; rssi_half_db = 124

// #define OPENOFDM_RX_RSSI_DBM_TH_DEFAULT (-85) //-85 will remove lots of false alarm. the best openwifi reported sensitivity is like -90/-92 (set it manually if conductive test with wifi tester)
#define OPENOFDM_RX_RSSI_DBM_TH_DEFAULT (-95) //due to performance is much better (can work around -90dBm), lower it from -85dBm to -95dBm
#define OPENOFDM_RX_DC_RUNNING_SUM_TH_INIT 64
#define OPENOFDM_RX_MIN_PLATEAU_INIT 100
#define OPENOFDM_RX_FFT_WIN_SHIFT_INIT 4
#define OPENOFDM_RX_SMALL_EQ_OUT_COUNTER_TH 48
#define OPENOFDM_RX_PHASE_OFFSET_ABS_TH 11

#define OPENWIFI_MAX_SIGNAL_LEN_TH 1700 //Packet longer  than this threshold will result in receiver early termination. It goes to openofdm_rx/xpu/rx_intf

#define OPENWIFI_MIN_SIGNAL_LEN_TH 14   //Packet shorter than this threshold will result in receiver early termination. It goes to openofdm_rx/xpu/rx_intf
                                        //due to CRC32, at least 4 bytes needed to push out expected CRC result

struct openofdm_rx_driver_api {
  u32 (*hw_init)(enum openofdm_rx_mode mode);

  u32 (*reg_read)(u32 reg);
  void (*reg_write)(u32 reg, u32 value);

  u32 (*OPENOFDM_RX_REG_STATE_HISTORY_read)(void);

  void (*OPENOFDM_RX_REG_MULTI_RST_write)(u32 value);
  void (*OPENOFDM_RX_REG_ENABLE_write)(u32 value);
  void (*OPENOFDM_RX_REG_POWER_THRES_write)(u32 value);
  void (*OPENOFDM_RX_REG_MIN_PLATEAU_write)(u32 value);
  void (*OPENOFDM_RX_REG_SOFT_DECODING_write)(u32 value);
  void (*OPENOFDM_RX_REG_FFT_WIN_SHIFT_write)(u32 value);
  void (*OPENOFDM_RX_REG_PHASE_OFFSET_ABS_TH_write)(u32 value);
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

#define XPU_REG_MULTI_RST_ADDR                (0*4)
#define XPU_REG_SRC_SEL_ADDR                  (1*4)
#define XPU_REG_TSF_LOAD_VAL_LOW_ADDR         (2*4)
#define XPU_REG_TSF_LOAD_VAL_HIGH_ADDR        (3*4)
#define XPU_REG_BAND_CHANNEL_ADDR             (4*4)
#define XPU_REG_DIFS_ADVANCE_ADDR             (5*4)
#define XPU_REG_FORCE_IDLE_MISC_ADDR          (6*4)
#define XPU_REG_RSSI_DB_CFG_ADDR              (7*4)
#define XPU_REG_LBT_TH_ADDR                   (8*4)
#define XPU_REG_CSMA_DEBUG_ADDR               (9*4)
#define XPU_REG_BB_RF_DELAY_ADDR             (10*4)
#define XPU_REG_ACK_CTL_MAX_NUM_RETRANS_ADDR  (11*4)
#define XPU_REG_AMPDU_ACTION_ADDR            (12*4)
#define XPU_REG_SPI_DISABLE_ADDR            (13*4)
#define XPU_REG_RECV_ACK_COUNT_TOP0_ADDR      (16*4)
#define XPU_REG_RECV_ACK_COUNT_TOP1_ADDR      (17*4)
#define XPU_REG_SEND_ACK_WAIT_TOP_ADDR        (18*4)
#define XPU_REG_CSMA_CFG_ADDR                 (19*4)

#define XPU_REG_SLICE_COUNT_TOTAL_ADDR    (20*4)
#define XPU_REG_SLICE_COUNT_START_ADDR    (21*4)
#define XPU_REG_SLICE_COUNT_END_ADDR      (22*4)

#define XPU_REG_CTS_TO_RTS_CONFIG_ADDR    (26*4)
#define XPU_REG_FILTER_FLAG_ADDR          (27*4)
#define XPU_REG_BSSID_FILTER_LOW_ADDR     (28*4)
#define XPU_REG_BSSID_FILTER_HIGH_ADDR    (29*4)
#define XPU_REG_MAC_ADDR_LOW_ADDR         (30*4)
#define XPU_REG_MAC_ADDR_HIGH_ADDR        (31*4)

#define XPU_REG_TSF_RUNTIME_VAL_LOW_ADDR  (58*4)
#define XPU_REG_TSF_RUNTIME_VAL_HIGH_ADDR (59*4)

#define XPU_REG_MAC_ADDR_READ_BACK_ADDR   (62*4)
#define XPU_REG_FPGA_GIT_REV_ADDR         (63*4)

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

  void (*XPU_REG_DIFS_ADVANCE_write)(u32 value);
  u32  (*XPU_REG_DIFS_ADVANCE_read)(void);

  void (*XPU_REG_FORCE_IDLE_MISC_write)(u32 value);
  u32  (*XPU_REG_FORCE_IDLE_MISC_read)(void);

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

  void (*XPU_REG_SLICE_COUNT_TOTAL_write)(u32 value);
  void (*XPU_REG_SLICE_COUNT_START_write)(u32 value);
  void (*XPU_REG_SLICE_COUNT_END_write)(u32 value);
  void (*XPU_REG_SLICE_COUNT_TOTAL1_write)(u32 value);
  void (*XPU_REG_SLICE_COUNT_START1_write)(u32 value);
  void (*XPU_REG_SLICE_COUNT_END1_write)(u32 value);

  u32 (*XPU_REG_SLICE_COUNT_TOTAL_read)(void);
  u32 (*XPU_REG_SLICE_COUNT_START_read)(void);
  u32 (*XPU_REG_SLICE_COUNT_END_read)(void);
  u32 (*XPU_REG_SLICE_COUNT_TOTAL1_read)(void);
  u32 (*XPU_REG_SLICE_COUNT_START1_read)(void);
  u32 (*XPU_REG_SLICE_COUNT_END1_read)(void);

  void (*XPU_REG_BB_RF_DELAY_write)(u32 value);
  
  void (*XPU_REG_ACK_CTL_MAX_NUM_RETRANS_write)(u32 value);
  u32  (*XPU_REG_ACK_CTL_MAX_NUM_RETRANS_read)(void);

  void (*XPU_REG_SPI_DISABLE_write)(u32 value); 
  u32  (*XPU_REG_SPI_DISABLE_read)(void);

  void (*XPU_REG_AMPDU_ACTION_write)(u32 value);
  u32  (*XPU_REG_AMPDU_ACTION_read)(void);

  void (*XPU_REG_MAC_ADDR_write)(u8 *mac_addr);
};

// #endif
