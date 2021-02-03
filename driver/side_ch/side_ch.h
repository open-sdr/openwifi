// Author: Xianjun Jiao
// SPDX-FileCopyrightText: 2019 UGent
// SPDX-License-Identifier: AGPL-3.0-or-later

// ---------------------------------------side channel-------------------------------
const char *side_ch_compatible_str = "sdr,side_ch";

//align with side_ch_control.v and all related user space, remote files
#define CSI_LEN 56 // length of single CSI
#define EQUALIZER_LEN (56-4) // for non HT, four {32767,32767} will be padded to achieve 52 (non HT should have 48)
#define HEADER_LEN 2 //timestamp and frequency offset

#define MAX_NUM_DMA_SYMBOL                         8192   //align with side_ch.v side_ch.h

#define SIDE_CH_REG_MULTI_RST_ADDR                 (0*4)
#define SIDE_CH_REG_CONFIG_ADDR                    (1*4)
#define SIDE_CH_REG_NUM_DMA_SYMBOL_ADDR            (2*4) //low 16bit to PS; high 16bit to PL
#define SIDE_CH_REG_IQ_CAPTURE_ADDR                (3*4)
#define SIDE_CH_REG_NUM_EQ_ADDR                    (4*4)
#define SIDE_CH_REG_FC_TARGET_ADDR                 (5*4)
#define SIDE_CH_REG_ADDR1_TARGET_ADDR              (6*4)
#define SIDE_CH_REG_ADDR2_TARGET_ADDR              (7*4)
#define SIDE_CH_REG_IQ_TRIGGER_ADDR                (8*4)
#define SIDE_CH_REG_RSSI_TH_ADDR                   (9*4)
#define SIDE_CH_REG_GAIN_TH_ADDR                   (10*4)
#define SIDE_CH_REG_PRE_TRIGGER_LEN_ADDR           (11*4)
#define SIDE_CH_REG_IQ_LEN_ADDR                    (12*4)

#define SIDE_CH_REG_M_AXIS_DATA_COUNT_ADDR         (20*4)
