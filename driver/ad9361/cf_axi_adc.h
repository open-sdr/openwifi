/*
 * ADI-AIM ADI ADC Interface Module
 *
 * Copyright 2012-2017 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 *
 * http://wiki.analog.com/resources/fpga/xilinx/fmc/ad9467
 * 
 * Modified by Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be
 * 
 */

#ifndef ADI_AXI_ADC_H_
#define ADI_AXI_ADC_H_

#define ADI_REG_VERSION		0x0000				/*Version and Scratch Registers */
#define ADI_VERSION(x)		(((x) & 0xffffffff) << 0)	/* RO, Version number. */
#define VERSION_IS(x,y,z)	((x) << 16 | (y) << 8 | (z))
#define ADI_REG_ID		0x0004			 	/*Version and Scratch Registers */
#define ADI_ID(x)		(((x) & 0xffffffff) << 0)   	/* RO, Instance identifier number. */
#define ADI_REG_SCRATCH		0x0008			 	/*Version and Scratch Registers */
#define ADI_SCRATCH(x)		(((x) & 0xffffffff) << 0)	/* RW, Scratch register. */

#define PCORE_VERSION(major, minor, letter) ((major << 16) | (minor << 8) | letter)
#define PCORE_VERSION_MAJOR(version) (version >> 16)
#define PCORE_VERSION_MINOR(version) ((version >> 8) & 0xff)
#define PCORE_VERSION_LETTER(version) (version & 0xff)

/* ADC COMMON */

#define ADI_REG_CONFIG 			0x000C
#define ADI_IQCORRECTION_DISABLE	(1 << 0)
#define ADI_DCFILTER_DISABLE		(1 << 1)
#define ADI_DATAFORMAT_DISABLE		(1 << 2)
#define ADI_USERPORTS_DISABLE		(1 << 3)
#define ADI_MODE_1R1T			(1 << 4)
#define ADI_SCALECORRECTION_ONLY	(1 << 5)
#define ADI_CMOS_OR_LVDS_N		(1 << 7)
#define ADI_PPS_RECEIVER_ENABLE		(1 << 8)

#define ADI_REG_RSTN			0x0040
#define ADI_RSTN				(1 << 0)
#define ADI_MMCM_RSTN 			(1 << 1)

#define ADI_REG_CNTRL			0x0044
#define ADI_R1_MODE			(1 << 2)
#define ADI_DDR_EDGESEL			(1 << 1)
#define ADI_PIN_MODE			(1 << 0)

#define ADI_REG_CLK_FREQ			0x0054
#define ADI_CLK_FREQ(x)			(((x) & 0xFFFFFFFF) << 0)
#define ADI_TO_CLK_FREQ(x)		(((x) >> 0) & 0xFFFFFFFF)

#define ADI_REG_CLK_RATIO		0x0058
#define ADI_CLK_RATIO(x)			(((x) & 0xFFFFFFFF) << 0)
#define ADI_TO_CLK_RATIO(x)		(((x) >> 0) & 0xFFFFFFFF)

#define ADI_REG_STATUS			0x005C
#define ADI_MUX_PN_ERR			(1 << 3)
#define ADI_MUX_PN_OOS			(1 << 2)
#define ADI_MUX_OVER_RANGE		(1 << 1)
#define ADI_STATUS			(1 << 0)

#define ADI_REG_DELAY_CNTRL		0x0060	/* <= v8.0 */
#define ADI_DELAY_SEL			(1 << 17)
#define ADI_DELAY_RWN			(1 << 16)
#define ADI_DELAY_ADDRESS(x)		(((x) & 0xFF) << 8)
#define ADI_TO_DELAY_ADDRESS(x)		(((x) >> 8) & 0xFF)
#define ADI_DELAY_WDATA(x)		(((x) & 0x1F) << 0)
#define ADI_TO_DELAY_WDATA(x)		(((x) >> 0) & 0x1F)

#define ADI_REG_DELAY_STATUS		0x0064 /* <= v8.0 */
#define ADI_DELAY_LOCKED			(1 << 9)
#define ADI_DELAY_STATUS			(1 << 8)
#define ADI_DELAY_RDATA(x)		(((x) & 0x1F) << 0)
#define ADI_TO_DELAY_RDATA(x)		(((x) >> 0) & 0x1F)

#define ADI_REG_DRP_CNTRL		0x0070
#define ADI_DRP_SEL			(1 << 29)
#define ADI_DRP_RWN			(1 << 28)
#define ADI_DRP_ADDRESS(x)		(((x) & 0xFFF) << 16)
#define ADI_TO_DRP_ADDRESS(x)		(((x) >> 16) & 0xFFF)
#define ADI_DRP_WDATA(x)			(((x) & 0xFFFF) << 0)
#define ADI_TO_DRP_WDATA(x)		(((x) >> 0) & 0xFFFF)

#define ADI_REG_DRP_STATUS		0x0074
#define ADI_DRP_STATUS			(1 << 16)
#define ADI_DRP_RDATA(x)			(((x) & 0xFFFF) << 0)
#define ADI_TO_DRP_RDATA(x)		(((x) >> 0) & 0xFFFF)

#define ADI_REG_DMA_STATUS		0x0088
#define ADI_DMA_OVF			(1 << 2)
#define ADI_DMA_UNF			(1 << 1)
#define ADI_DMA_STATUS			(1 << 0)

#define ADI_REG_DMA_BUSWIDTH		0x008C
#define ADI_DMA_BUSWIDTH(x)		(((x) & 0xFFFFFFFF) << 0)
#define ADI_TO_DMA_BUSWIDTH(x)		(((x) >> 0) & 0xFFFFFFFF)

#define ADI_REG_USR_CNTRL_1		0x00A0
#define ADI_USR_CHANMAX(x)		(((x) & 0xFF) << 0)
#define ADI_TO_USR_CHANMAX(x)		(((x) >> 0) & 0xFF)

#define ADI_REG_GP_CONTROL		0x00BC

#define ADI_REG_CLOCKS_PER_PPS		0x00C0
#define ADI_REG_CLOCKS_PER_PPS_STATUS	0x00C4
#define ADI_CLOCKS_PER_PPS_STAT_INVAL	(1 << 0)

/* ADC CHANNEL */

#define ADI_REG_CHAN_CNTRL(c)		(0x0400 + (c) * 0x40)
#define ADI_PN_SEL			(1 << 10) /* !v8.0 */
#define ADI_IQCOR_ENB			(1 << 9)
#define ADI_DCFILT_ENB			(1 << 8)
#define ADI_FORMAT_SIGNEXT		(1 << 6)
#define ADI_FORMAT_TYPE			(1 << 5)
#define ADI_FORMAT_ENABLE		(1 << 4)
#define ADI_PN23_TYPE			(1 << 1) /* !v8.0 */
#define ADI_ENABLE			(1 << 0)

#define ADI_REG_CHAN_STATUS(c)		(0x0404 + (c) * 0x40)
#define ADI_PN_ERR			(1 << 2)
#define ADI_PN_OOS			(1 << 1)
#define ADI_OVER_RANGE			(1 << 0)

#define ADI_REG_CHAN_CNTRL_1(c)		(0x0410 + (c) * 0x40)
#define ADI_DCFILT_OFFSET(x)		(((x) & 0xFFFF) << 16)
#define ADI_TO_DCFILT_OFFSET(x)		(((x) >> 16) & 0xFFFF)
#define ADI_DCFILT_COEFF(x)		(((x) & 0xFFFF) << 0)
#define ADI_TO_DCFILT_COEFF(x)		(((x) >> 0) & 0xFFFF)

#define ADI_REG_CHAN_CNTRL_2(c)		(0x0414 + (c) * 0x40)
#define ADI_IQCOR_COEFF_1(x)		(((x) & 0xFFFF) << 16)
#define ADI_TO_IQCOR_COEFF_1(x)		(((x) >> 16) & 0xFFFF)
#define ADI_IQCOR_COEFF_2(x)		(((x) & 0xFFFF) << 0)
#define ADI_TO_IQCOR_COEFF_2(x)		(((x) >> 0) & 0xFFFF)

#define ADI_REG_CHAN_CNTRL_3(c)		(0x0418 + (c) * 0x40) /* v8.0 */
#define ADI_ADC_PN_SEL(x)		(((x) & 0xF) << 16)
#define ADI_TO_ADC_PN_SEL(x)		(((x) >> 16) & 0xF)
#define ADI_ADC_DATA_SEL(x)		(((x) & 0xF) << 0)
#define ADI_TO_ADC_DATA_SEL(x)		(((x) >> 0) & 0xF)

enum adc_pn_sel {
	ADC_PN9 = 0,
	ADC_PN23A = 1,
	ADC_PN7 = 4,
	ADC_PN15 = 5,
	ADC_PN23 = 6,
	ADC_PN31 = 7,
	ADC_PN_CUSTOM = 9,
	ADC_PN_OFF = 10,
};

enum adc_data_sel {
	ADC_DATA_SEL_NORM,
	ADC_DATA_SEL_LB, /* DAC loopback */
	ADC_DATA_SEL_RAMP, /* TBD */
};

#define ADI_REG_CHAN_USR_CNTRL_1(c)		(0x0420 + (c) * 0x40)
#define ADI_USR_DATATYPE_BE			(1 << 25)
#define ADI_USR_DATATYPE_SIGNED			(1 << 24)
#define ADI_USR_DATATYPE_SHIFT(x)		(((x) & 0xFF) << 16)
#define ADI_TO_USR_DATATYPE_SHIFT(x)		(((x) >> 16) & 0xFF)
#define ADI_USR_DATATYPE_TOTAL_BITS(x)		(((x) & 0xFF) << 8)
#define ADI_TO_USR_DATATYPE_TOTAL_BITS(x)	(((x) >> 8) & 0xFF)
#define ADI_USR_DATATYPE_BITS(x)			(((x) & 0xFF) << 0)
#define ADI_TO_USR_DATATYPE_BITS(x)		(((x) >> 0) & 0xFF)

#define ADI_REG_CHAN_USR_CNTRL_2(c)		(0x0424 + (c) * 0x40)
#define ADI_USR_DECIMATION_M(x)			(((x) & 0xFFFF) << 16)
#define ADI_TO_USR_DECIMATION_M(x)		(((x) >> 16) & 0xFFFF)
#define ADI_USR_DECIMATION_N(x)			(((x) & 0xFFFF) << 0)
#define ADI_TO_USR_DECIMATION_N(x)		(((x) >> 0) & 0xFFFF)

#define ADI_REG_ADC_DP_DISABLE 			0x00C0

/* PCORE Version > 8.00 */
#define ADI_REG_DELAY(l)				(0x0800 + (l) * 0x4)

/* debugfs direct register access */
#define DEBUGFS_DRA_PCORE_REG_MAGIC	0x80000000

#define AXIADC_MAX_CHANNEL		16

#include <linux/spi/spi.h>
#include <linux/clk/clkscale.h>

struct axiadc_chip_info {
	char				*name;
	unsigned			num_channels;
	unsigned 		num_shadow_slave_channels;
	const unsigned long 	*scan_masks;
	const int			(*scale_table)[2];
	int				num_scales;
	int				max_testmode;
	unsigned long			max_rate;
	struct iio_chan_spec		channel[AXIADC_MAX_CHANNEL];
};

struct axiadc_state {
	struct device 			*dev_spi;
	struct iio_info			iio_info;
	struct clk 			*clk;
	size_t				regs_size;
	void __iomem			*regs;
	void __iomem			*slave_regs;
	unsigned				max_usr_channel;
	unsigned			adc_def_output_mode;
	unsigned			max_count;
	unsigned			id;
	unsigned			pcore_version;
	unsigned			decimation_factor;
	bool				dp_disable;
	unsigned long long		adc_clk;
	unsigned			have_slave_channels;

	struct iio_hw_consumer		*frontend;

	struct iio_chan_spec		channels[AXIADC_MAX_CHANNEL];
};

struct axiadc_converter {
	struct spi_device 	*spi;
	struct clk 		*clk;
	struct clock_scale		adc_clkscale;
	struct clk		*lane_clk;
	struct clk		*sysref_clk;
	void 			*phy;
	struct gpio_desc		*pwrdown_gpio;
	struct gpio_desc		*reset_gpio;
	unsigned			id;
	unsigned			adc_output_mode;
	unsigned 		testmode[AXIADC_MAX_CHANNEL];
	unsigned			scratch_reg[AXIADC_MAX_CHANNEL];
	unsigned long 		adc_clk;
	const struct axiadc_chip_info	*chip_info;

	bool			sample_rate_read_only;

	int (*reg_access)(struct iio_dev *indio_dev, unsigned int reg,
		unsigned int writeval, unsigned int *readval);
	int (*setup)(struct spi_device *spi, unsigned mode);

	struct iio_chan_spec const	*channels;
	int				num_channels;
	const struct attribute_group	*attrs;
	struct iio_dev 	*indio_dev;
	int (*read_raw)(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask);

	int (*write_raw)(struct iio_dev *indio_dev,
			 struct iio_chan_spec const *chan,
			 int val,
			 int val2,
			 long mask);

	int (*read_event_value)(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			enum iio_event_type type,
			enum iio_event_direction dir,
			enum iio_event_info info,
			int *val,
			int *val2);

	int (*write_event_value)(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			enum iio_event_type type,
			enum iio_event_direction dir,
			enum iio_event_info info,
			int val,
			int val2);

	int (*read_event_config)(struct iio_dev *indio_dev,
			const struct iio_chan_spec *chan,
			enum iio_event_type type,
			enum iio_event_direction dir);

	int (*write_event_config)(struct iio_dev *indio_dev,
			const struct iio_chan_spec *chan,
			enum iio_event_type type,
			enum iio_event_direction dir,
			int state);

	int (*post_setup)(struct iio_dev *indio_dev);
	int (*set_pnsel)(struct iio_dev *indio_dev, unsigned chan,
			enum adc_pn_sel sel);
};



static inline struct axiadc_converter *to_converter(struct device *dev)
{
	struct axiadc_converter *conv = spi_get_drvdata(to_spi_device(dev));

	if (conv)
		return conv;

	return ERR_PTR(-ENODEV);
};

struct axiadc_spidev {
	struct device_node *of_nspi;
	struct device *dev_spi;
};

/*
 * IO accessors
 */

static inline void axiadc_write(struct axiadc_state *st, unsigned reg, unsigned val)
{
	iowrite32(val, st->regs + reg);
}

static inline unsigned int axiadc_read(struct axiadc_state *st, unsigned reg)
{
	return ioread32(st->regs + reg);
}

static inline void axiadc_slave_write(struct axiadc_state *st, unsigned reg, unsigned val)
{
	iowrite32(val, st->slave_regs + reg);
}

static inline unsigned int axiadc_slave_read(struct axiadc_state *st, unsigned reg)
{
	return ioread32(st->slave_regs + reg);
}


static inline void axiadc_idelay_set(struct axiadc_state *st,
				unsigned lane, unsigned val)
{
	if (PCORE_VERSION_MAJOR(st->pcore_version) > 8) {
		axiadc_write(st, ADI_REG_DELAY(lane), val);
	} else {
		axiadc_write(st, ADI_REG_DELAY_CNTRL, 0);
		axiadc_write(st, ADI_REG_DELAY_CNTRL,
				ADI_DELAY_ADDRESS(lane)
				| ADI_DELAY_WDATA(val)
				| ADI_DELAY_SEL);
	}
}

int axiadc_set_pnsel(struct axiadc_state *st, int channel, enum adc_pn_sel sel);
enum adc_pn_sel axiadc_get_pnsel(struct axiadc_state *st,
			       int channel, const char **name);

int axiadc_configure_ring_stream(struct iio_dev *indio_dev,
	const char *dma_name);
void axiadc_unconfigure_ring_stream(struct iio_dev *indio_dev);

#endif /* ADI_AXI_ADC_H_ */
