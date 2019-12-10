/*
 * AD9361
 *
 * Copyright 2013-2018 Analog Devices Inc.
 * 
 * Modified by Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be
 * Licensed under the GPL-2.
 */

#ifndef IIO_FREQUENCY_AD9361_H_
#define IIO_FREQUENCY_AD9361_H_

//#define IIO_AD9361_USE_PRIVATE_H_

#include "ad9361_regs.h"
//#include "ad9361_private.h"

enum ad9361_clocks {
	BB_REFCLK,
	RX_REFCLK,
	TX_REFCLK,
	BBPLL_CLK,
	ADC_CLK,
	R2_CLK,
	R1_CLK,
	CLKRF_CLK,
	RX_SAMPL_CLK,
	DAC_CLK,
	T2_CLK,
	T1_CLK,
	CLKTF_CLK,
	TX_SAMPL_CLK,
	RX_RFPLL_INT,
	TX_RFPLL_INT,
	RX_RFPLL_DUMMY,
	TX_RFPLL_DUMMY,
	RX_RFPLL,
	TX_RFPLL,
	NUM_AD9361_CLKS,
};

enum debugfs_cmd {
	DBGFS_NONE,
	DBGFS_INIT,
	DBGFS_LOOPBACK,
	DBGFS_BIST_PRBS,
	DBGFS_BIST_TONE,
	DBGFS_BIST_DT_ANALYSIS,
	DBGFS_RXGAIN_1,
	DBGFS_RXGAIN_2,
	DBGFS_MCS,
	DBGFS_CAL_SW_CTRL,
	DBGFS_DIGITAL_TUNE,
};

enum dig_tune_flags {
	BE_VERBOSE = 1,
	BE_MOREVERBOSE = 2,
	DO_IDELAY = 4,
	DO_ODELAY = 8,
	SKIP_STORE_RESULT = 16,
	RESTORE_DEFAULT = 32,
};

enum ad9361_bist_mode {
	BIST_DISABLE,
	BIST_INJ_TX,
	BIST_INJ_RX,
};

enum {
	ID_AD9361,
	ID_AD9364,
	ID_AD9361_2,
	ID_AD9363A,
};

enum rx_port_sel {
	RX_A_BALANCED,	/* 0 = (RX1A_N &  RX1A_P) and (RX2A_N & RX2A_P) enabled; balanced */
	RX_B_BALANCED,  /* 1 = (RX1B_N &  RX1B_P) and (RX2B_N & RX2B_P) enabled; balanced */
	RX_C_BALANCED,  /* 2 = (RX1C_N &  RX1C_P) and (RX2C_N & RX2C_P) enabled; balanced */
	RX_A_N,		/* 3 = RX1A_N and RX2A_N enabled; unbalanced */
	RX_A_P,		/* 4 = RX1A_P and RX2A_P enabled; unbalanced */
	RX_B_N,		/* 5 = RX1B_N and RX2B_N enabled; unbalanced */
	RX_B_P,		/* 6 = RX1B_P and RX2B_P enabled; unbalanced */
	RX_C_N,		/* 7 = RX1C_N and RX2C_N enabled; unbalanced */
	RX_C_P,		/* 8 = RX1C_P and RX2C_P enabled; unbalanced */
	TX_MON1,	/* 9 = TX_MON1 enabled */
	TX_MON2,	/* 10 = TX_MON2 enabled */
	TX_MON1_2,	/* 11 = TX_MON1 & TX_MON2 enabled */
};

enum tx_port_sel {
	TX_A,
	TX_B,
};

enum digital_tune_skip_mode {
	TUNE_RX_TX,
	SKIP_TX,
	SKIP_ALL,
};

enum rssi_restart_mode {
	AGC_IN_FAST_ATTACK_MODE_LOCKS_THE_GAIN,
	EN_AGC_PIN_IS_PULLED_HIGH,
	ENTERS_RX_MODE,
	GAIN_CHANGE_OCCURS,
	SPI_WRITE_TO_REGISTER,
	GAIN_CHANGE_OCCURS_OR_EN_AGC_PIN_PULLED_HIGH,
};

struct ctrl_outs_control {
	u8			index;
	u8			en_mask;
};

struct rssi_control {
	enum rssi_restart_mode restart_mode;
	bool rssi_unit_is_rx_samples;	/* default unit is time */
	u32 rssi_delay;
	u32 rssi_wait;
	u32 rssi_duration;
};

struct rf_rssi {
	u32 ant;		/* Antenna number for which RSSI is reported */
	u32 symbol;		/* Runtime RSSI */
	u32 preamble;		/* Initial RSSI */
	s32 multiplier;	/* Multiplier to convert reported RSSI */
	u8 duration;		/* Duration to be considered for measuring */
};

struct ad9361_rf_phy;
struct ad9361_debugfs_entry {
	struct ad9361_rf_phy *phy;
	const char *propname;
	void *out_value;
	u32 val;
	u8 size;
	u8 cmd;
};

struct ad9361_dig_tune_data {
	u32 bist_loopback_mode;
	u32 bist_config;
	u32 ensm_state;
	u8 skip_mode;
};

struct refclk_scale {
	struct clk_hw		hw;
	struct spi_device	*spi;
	struct ad9361_rf_phy	*phy;
	unsigned long		rate;
	u32			mult;
	u32			div;
	enum ad9361_clocks 	source;
};

struct ad9361_rf_phy_state;
struct ad9361_ext_band_ctl;

struct ad9361_rf_phy {
	struct spi_device 	*spi;
	struct clk 		*clk_refin;
	struct clk 		*clk_ext_lo_rx;
	struct clk 		*clk_ext_lo_tx;
	struct clk 		*clks[NUM_AD9361_CLKS];
	struct notifier_block   clk_nb_tx;
	struct notifier_block   clk_nb_rx;
	struct refclk_scale	clk_priv[NUM_AD9361_CLKS];
	struct clk_onecell_data	clk_data;
	struct ad9361_phy_platform_data *pdata;
	struct ad9361_debugfs_entry debugfs_entry[181];
	struct bin_attribute 	bin;
	struct bin_attribute 	bin_gt;
	struct iio_dev 		*indio_dev;
	struct work_struct 	work;
	struct completion       complete;
	struct gain_table_info  *gt_info;
	char			*bin_attr_buf;
	u32 			ad9361_debugfs_entry_index;

	struct ad9361_ext_band_ctl	*ext_band_ctl;
	struct ad9361_rf_phy_state	*state;
};
int ad9361_ctrl_outs_setup(struct ad9361_rf_phy *phy, struct ctrl_outs_control *ctrl);
int ad9361_clk_set_rate(struct clk *clk, unsigned long rate);
int ad9361_rssi_setup(struct ad9361_rf_phy *phy,
			     struct rssi_control *ctrl,
			     bool is_update);
int ad9361_read_rssi(struct ad9361_rf_phy *phy, struct rf_rssi *rssi);
int ad9361_update_rf_bandwidth(struct ad9361_rf_phy *phy,u32 rf_rx_bw, u32 rf_tx_bw);

ssize_t ad9361_dig_interface_timing_analysis(struct ad9361_rf_phy *phy,
						   char *buf, unsigned buflen);
int ad9361_hdl_loopback(struct ad9361_rf_phy *phy, bool enable);
int ad9361_register_axi_converter(struct ad9361_rf_phy *phy);
struct ad9361_rf_phy* ad9361_spi_to_phy(struct spi_device *spi);
int ad9361_spi_read(struct spi_device *spi, u32 reg);
int ad9361_spi_write(struct spi_device *spi, u32 reg, u32 val);
int ad9361_bist_loopback(struct ad9361_rf_phy *phy, unsigned mode);
int ad9361_bist_prbs(struct ad9361_rf_phy *phy, enum ad9361_bist_mode mode);
int ad9361_find_opt(u8 *field, u32 size, u32 *ret_start);
int ad9361_ensm_mode_disable_pinctrl(struct ad9361_rf_phy *phy);
int ad9361_ensm_mode_restore_pinctrl(struct ad9361_rf_phy *phy);
void ad9361_ensm_force_state(struct ad9361_rf_phy *phy, u8 ensm_state);
void ad9361_ensm_restore_state(struct ad9361_rf_phy *phy, u8 ensm_state);
void ad9361_ensm_restore_prev_state(struct ad9361_rf_phy *phy);
int ad9361_set_trx_clock_chain_freq(struct ad9361_rf_phy *phy,
					  unsigned long freq);
int ad9361_set_trx_clock_chain_default(struct ad9361_rf_phy *phy);
int ad9361_dig_tune(struct ad9361_rf_phy *phy, unsigned long max_freq,
			   enum dig_tune_flags flags);
int ad9361_tx_mute(struct ad9361_rf_phy *phy, u32 state);
int ad9361_get_tx_atten(struct ad9361_rf_phy *phy, u32 tx_num);
int ad9361_set_tx_atten(struct ad9361_rf_phy *phy, u32 atten_mdb, bool tx1, bool tx2, bool immed);
int ad9361_write_bist_reg(struct ad9361_rf_phy *phy, u32 val);
bool ad9361_uses_rx2tx2(struct ad9361_rf_phy *phy);
int ad9361_get_dig_tune_data(struct ad9361_rf_phy *phy,
			     struct ad9361_dig_tune_data *data);
int ad9361_read_clock_data_delays(struct ad9361_rf_phy *phy);
int ad9361_write_clock_data_delays(struct ad9361_rf_phy *phy);
bool ad9361_uses_lvds_mode(struct ad9361_rf_phy *phy);
int ad9361_set_rx_port(struct ad9361_rf_phy *phy, enum rx_port_sel sel);
int ad9361_set_tx_port(struct ad9361_rf_phy *phy, enum tx_port_sel sel);

#ifdef CONFIG_AD9361_EXT_BAND_CONTROL
int ad9361_register_ext_band_control(struct ad9361_rf_phy *phy);
int ad9361_adjust_rx_ext_band_settings(struct ad9361_rf_phy *phy, u64 freq);
int ad9361_adjust_tx_ext_band_settings(struct ad9361_rf_phy *phy, u64 freq);
void ad9361_unregister_ext_band_control(struct ad9361_rf_phy *phy);
#else
static inline int ad9361_register_ext_band_control(struct ad9361_rf_phy *phy)
{
	return 0;
}
static inline int ad9361_adjust_rx_ext_band_settings(
		struct ad9361_rf_phy *phy, u64 freq)
{
	return 0;
}
static inline int ad9361_adjust_tx_ext_band_settings(
		struct ad9361_rf_phy *phy, u64 freq)
{
	return 0;
}
static inline void ad9361_unregister_ext_band_control(
		struct ad9361_rf_phy *phy)
{}
#endif

#endif

