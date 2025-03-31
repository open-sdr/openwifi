/*
 * openwifi side channel driver
 * Author: Xianjun Jiao
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
#include <linux/dmaengine.h>

#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include "side_ch.h"

static int num_eq_init = 8; // should be 0~8
static int iq_len_init = 0; //if iq_len>0, iq capture enabled, csi disabled

module_param(num_eq_init, int, 0);
MODULE_PARM_DESC(num_eq_init, "num_eq_init. 0~8. number of equalizer output (52 each) appended to CSI");

module_param(iq_len_init, int, 0);
MODULE_PARM_DESC(iq_len_init, "iq_len_init. if iq_len_init>0, iq capture enabled, csi disabled");

static void __iomem *base_addr; // to store driver specific base address needed for mmu to translate virtual address to physical address in our FPGA design

struct dma_chan *chan_to_pl = NULL;
struct dma_chan *chan_to_ps = NULL;
u8 *side_info_buf = NULL;
dma_cookie_t chan_to_ps_cookie;
const int max_side_info_buf_size = MAX_NUM_DMA_SYMBOL*8;

/* IO accessors */
static inline u32 reg_read(u32 reg)
{
	return ioread32(base_addr + reg);
}

static inline void reg_write(u32 reg, u32 value)
{
	iowrite32(value, base_addr + reg);
}

static inline void SIDE_CH_REG_MULTI_RST_write(u32 Data) {
	reg_write(SIDE_CH_REG_MULTI_RST_ADDR, Data);
}

static inline u32 SIDE_CH_REG_CONFIG_read(void){
	return reg_read(SIDE_CH_REG_CONFIG_ADDR);
}

static inline void SIDE_CH_REG_CONFIG_write(u32 value){
	reg_write(SIDE_CH_REG_CONFIG_ADDR, value);
}

static inline u32 SIDE_CH_REG_NUM_DMA_SYMBOL_read(void){
	return reg_read(SIDE_CH_REG_NUM_DMA_SYMBOL_ADDR);
}

static inline void SIDE_CH_REG_NUM_DMA_SYMBOL_write(u32 value){
	reg_write(SIDE_CH_REG_NUM_DMA_SYMBOL_ADDR, value);
}

static inline u32 SIDE_CH_REG_IQ_CAPTURE_read(void){
	return reg_read(SIDE_CH_REG_IQ_CAPTURE_ADDR);
}

static inline void SIDE_CH_REG_IQ_CAPTURE_write(u32 value){
	reg_write(SIDE_CH_REG_IQ_CAPTURE_ADDR, value);
}

static inline u32 SIDE_CH_REG_NUM_EQ_read(void){
	return reg_read(SIDE_CH_REG_NUM_EQ_ADDR);
}

static inline void SIDE_CH_REG_NUM_EQ_write(u32 value){
	reg_write(SIDE_CH_REG_NUM_EQ_ADDR, value);
}

static inline u32 SIDE_CH_REG_FC_TARGET_read(void){
	return reg_read(SIDE_CH_REG_FC_TARGET_ADDR);
}

static inline void SIDE_CH_REG_FC_TARGET_write(u32 value){
	reg_write(SIDE_CH_REG_FC_TARGET_ADDR, value);
}

static inline u32 SIDE_CH_REG_ADDR1_TARGET_read(void){
	return reg_read(SIDE_CH_REG_ADDR1_TARGET_ADDR);
}

static inline void SIDE_CH_REG_ADDR1_TARGET_write(u32 value){
	reg_write(SIDE_CH_REG_ADDR1_TARGET_ADDR, value);
}

static inline u32 SIDE_CH_REG_ADDR2_TARGET_read(void){
	return reg_read(SIDE_CH_REG_ADDR2_TARGET_ADDR);
}

static inline void SIDE_CH_REG_ADDR2_TARGET_write(u32 value){
	reg_write(SIDE_CH_REG_ADDR2_TARGET_ADDR, value);
}

static inline u32 SIDE_CH_REG_IQ_TRIGGER_read(void){
	return reg_read(SIDE_CH_REG_IQ_TRIGGER_ADDR);
}

static inline void SIDE_CH_REG_IQ_TRIGGER_write(u32 value){
	reg_write(SIDE_CH_REG_IQ_TRIGGER_ADDR, value);
}

static inline u32 SIDE_CH_REG_RSSI_TH_read(void){
	return reg_read(SIDE_CH_REG_RSSI_TH_ADDR);
}

static inline void SIDE_CH_REG_RSSI_TH_write(u32 value){
	reg_write(SIDE_CH_REG_RSSI_TH_ADDR, value);
}

static inline u32 SIDE_CH_REG_GAIN_TH_read(void){
	return reg_read(SIDE_CH_REG_GAIN_TH_ADDR);
}

static inline void SIDE_CH_REG_GAIN_TH_write(u32 value){
	reg_write(SIDE_CH_REG_GAIN_TH_ADDR, value);
}

static inline u32 SIDE_CH_REG_PRE_TRIGGER_LEN_read(void){
	return reg_read(SIDE_CH_REG_PRE_TRIGGER_LEN_ADDR);
}

static inline void SIDE_CH_REG_PRE_TRIGGER_LEN_write(u32 value){
	reg_write(SIDE_CH_REG_PRE_TRIGGER_LEN_ADDR, value);
}

static inline u32 SIDE_CH_REG_IQ_LEN_read(void){
	return reg_read(SIDE_CH_REG_IQ_LEN_ADDR);
}

static inline void SIDE_CH_REG_IQ_LEN_write(u32 value){
	reg_write(SIDE_CH_REG_IQ_LEN_ADDR, value);
}

static inline u32 SIDE_CH_REG_M_AXIS_DATA_COUNT_read(void){
	return reg_read(SIDE_CH_REG_M_AXIS_DATA_COUNT_ADDR);
}

static inline void SIDE_CH_REG_M_AXIS_DATA_COUNT_write(u32 value){
	reg_write(SIDE_CH_REG_M_AXIS_DATA_COUNT_ADDR, value);
}

static const struct of_device_id dev_of_ids[] = {
	{ .compatible = "sdr,side_ch", },
	{}
};
MODULE_DEVICE_TABLE(of, dev_of_ids);

static void chan_to_ps_callback(void *completion)
{
	complete(completion);
}

#if 0
static void chan_to_pl_callback(void *completion)
{
	complete(completion);
}

static int dma_loopback_test(int num_test, int num_dma_symbol) {
	int i, err = 0;

	// -----------dma loop back test-------------------------
	enum dma_status status;
	enum dma_ctrl_flags flags;
	u8 *src_buf, *dst_buf;
	// int num_dma_symbol = 16;
	int test_buf_size = num_dma_symbol*8;
	dma_addr_t src_buf_dma;
	dma_addr_t dst_buf_dma;
	struct dma_device *chan_to_pl_dev = chan_to_pl->device;
	struct dma_device *chan_to_ps_dev = chan_to_ps->device;
	struct scatterlist chan_to_pl_sg[1];
	struct scatterlist chan_to_ps_sg[1];
	dma_cookie_t chan_to_pl_cookie;
	dma_cookie_t chan_to_ps_cookie;
	struct completion chan_to_pl_cmp;
	struct completion chan_to_ps_cmp;
	struct dma_async_tx_descriptor *chan_to_pl_d = NULL;
	struct dma_async_tx_descriptor *chan_to_ps_d = NULL;
	unsigned long chan_to_ps_tmo =	msecs_to_jiffies(300000);
	unsigned long chan_to_pl_tmo =  msecs_to_jiffies(30000);
	int test_idx;

	for (test_idx=0; test_idx<num_test; test_idx++) {
		printk("%s test_idx %d\n", side_ch_compatible_str, test_idx);
		//set number of dma symbols expected to pl and ps
		SIDE_CH_REG_NUM_DMA_SYMBOL_write((num_dma_symbol<<16)|num_dma_symbol);

		src_buf = kmalloc(test_buf_size, GFP_KERNEL);
		if (!src_buf)
			goto err_src_buf;
		
		dst_buf = kmalloc(test_buf_size, GFP_KERNEL);
		if (!dst_buf)
			goto err_dst_buf;

		// test buf init
		for (i=0; i<test_buf_size; i++) {
			src_buf[i] = (test_idx+test_buf_size-i-1);
			dst_buf[i] = 0;
		}

		set_user_nice(current, 10);
		flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;

		src_buf_dma = dma_map_single(chan_to_pl_dev->dev, src_buf, test_buf_size, DMA_MEM_TO_DEV);
		if (dma_mapping_error(chan_to_pl_dev->dev, src_buf_dma)) {
			printk("%s dma_loopback_test WARNING chan_to_pl_dev DMA mapping error\n", side_ch_compatible_str);
			goto err_src_buf_dma_mapping;
		}

		dst_buf_dma = dma_map_single(chan_to_ps_dev->dev, dst_buf, test_buf_size, DMA_DEV_TO_MEM);
		if (dma_mapping_error(chan_to_ps_dev->dev, dst_buf_dma)) {
			printk("%s dma_loopback_test WARNING chan_to_ps_dev DMA mapping error\n", side_ch_compatible_str);
			goto err_dst_buf_dma_mapping;
		}

		sg_init_table(chan_to_ps_sg, 1);
		sg_init_table(chan_to_pl_sg, 1);

		sg_dma_address(&chan_to_ps_sg[0]) = dst_buf_dma;
		sg_dma_address(&chan_to_pl_sg[0]) = src_buf_dma;

		sg_dma_len(&chan_to_ps_sg[0]) = test_buf_size;
		sg_dma_len(&chan_to_pl_sg[0]) = test_buf_size;

		chan_to_ps_d = chan_to_ps_dev->device_prep_slave_sg(chan_to_ps, chan_to_ps_sg, 1, DMA_DEV_TO_MEM, flags, NULL);
		chan_to_pl_d = chan_to_pl_dev->device_prep_slave_sg(chan_to_pl, chan_to_pl_sg, 1, DMA_MEM_TO_DEV, flags, NULL);

		if (!chan_to_ps_d || !chan_to_pl_d) {
			printk("%s dma_loopback_test WARNING !chan_to_ps_d || !chan_to_pl_d\n", side_ch_compatible_str);
			goto err_dst_buf_with_unmap;
		}

		init_completion(&chan_to_pl_cmp);
		chan_to_pl_d->callback = chan_to_pl_callback;
		chan_to_pl_d->callback_param = &chan_to_pl_cmp;
		chan_to_pl_cookie = chan_to_pl_d->tx_submit(chan_to_pl_d);

		init_completion(&chan_to_ps_cmp);
		chan_to_ps_d->callback = chan_to_ps_callback;
		chan_to_ps_d->callback_param = &chan_to_ps_cmp;
		chan_to_ps_cookie = chan_to_ps_d->tx_submit(chan_to_ps_d);

		if (dma_submit_error(chan_to_pl_cookie) ||	dma_submit_error(chan_to_ps_cookie)) {
			printk("%s dma_loopback_test WARNING dma_submit_error\n", side_ch_compatible_str);
			goto err_dst_buf_with_unmap;
		}

		dma_async_issue_pending(chan_to_pl);
		dma_async_issue_pending(chan_to_ps);

		chan_to_pl_tmo = wait_for_completion_timeout(&chan_to_pl_cmp, chan_to_pl_tmo);

		status = dma_async_is_tx_complete(chan_to_pl, chan_to_pl_cookie, NULL, NULL);
		if (chan_to_pl_tmo == 0) {
			printk("%s dma_loopback_test chan_to_pl_tmo == 0\n", side_ch_compatible_str);
			goto err_dst_buf_with_unmap;
		} else if (status != DMA_COMPLETE) {
			printk("%s dma_loopback_test chan_to_pl status != DMA_COMPLETE\n", side_ch_compatible_str);
			goto err_dst_buf_with_unmap;
		}

		chan_to_ps_tmo = wait_for_completion_timeout(&chan_to_ps_cmp, chan_to_ps_tmo);
		status = dma_async_is_tx_complete(chan_to_ps, chan_to_ps_cookie, NULL, NULL);
		if (chan_to_ps_tmo == 0) {
			printk("%s dma_loopback_test chan_to_ps_tmo == 0\n", side_ch_compatible_str);
			goto err_dst_buf_with_unmap;
		} else if (status != DMA_COMPLETE) {
			printk("%s dma_loopback_test chan_to_ps status != DMA_COMPLETE\n", side_ch_compatible_str);
			goto err_dst_buf_with_unmap;
		}

		dma_unmap_single(chan_to_pl_dev->dev, src_buf_dma, test_buf_size, DMA_MEM_TO_DEV);
		dma_unmap_single(chan_to_ps_dev->dev, dst_buf_dma, test_buf_size, DMA_DEV_TO_MEM);

		// test buf verification
		for (i=0; i<test_buf_size; i++) {
			//printk("%d ", dst_buf[i]);
			if ( dst_buf[i] != ((test_idx+test_buf_size-i-1)%256) )
				break;
		}
		printk("\n");
		printk("%s dma_loopback_test buf verification end idx %d (test_buf_size %d)\n", side_ch_compatible_str, i, test_buf_size);

		kfree(src_buf);
		kfree(dst_buf);
	}

	printk("%s dma_loopback_test err %d\n", side_ch_compatible_str, err);
	return(err);

err_dst_buf_with_unmap:
	dma_unmap_single(chan_to_ps_dev->dev, dst_buf_dma, test_buf_size, DMA_DEV_TO_MEM);
	
err_dst_buf_dma_mapping:
	dma_unmap_single(chan_to_pl_dev->dev, src_buf_dma, test_buf_size, DMA_MEM_TO_DEV);

err_src_buf_dma_mapping:

err_dst_buf:
	err = -4;
	kfree((void*)dst_buf);

err_src_buf:
	err = -3;
	kfree(src_buf);

	return(err);
}
#endif

static int init_side_channel(void) {
	side_info_buf = kmalloc(max_side_info_buf_size, GFP_KERNEL);
	if (!side_info_buf)
		return(-1);

	return(0);
}

static int get_side_info(int num_eq, int iq_len) {
	// int err = 0;//, i;
	struct scatterlist chan_to_ps_sg[1];
	enum dma_status status;
	enum dma_ctrl_flags flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
	int num_dma_symbol, num_dma_symbol_per_trans, side_info_buf_size;
	dma_addr_t side_info_buf_dma;
	struct dma_device *chan_to_ps_dev = chan_to_ps->device;
	struct completion chan_to_ps_cmp;
	struct dma_async_tx_descriptor *chan_to_ps_d = NULL;
	unsigned long chan_to_ps_tmo =	msecs_to_jiffies(100);

	if (side_info_buf==NULL) {
		printk("%s get_side_info WARNING side_info_buf==NULL\n", side_ch_compatible_str);
		return(-1);
	}

	status = dma_async_is_tx_complete(chan_to_ps, chan_to_ps_cookie, NULL, NULL);
	if (status!=DMA_COMPLETE) {
		printk("%s get_side_info WARNING status!=DMA_COMPLETE\n", side_ch_compatible_str);
		return(-1);
	}

	set_user_nice(current, 10);

	if (iq_len>0)
		num_dma_symbol_per_trans = 1+iq_len;
	else
		num_dma_symbol_per_trans = HEADER_LEN + CSI_LEN + num_eq*EQUALIZER_LEN;
	//set number of dma symbols expected to ps
	num_dma_symbol = SIDE_CH_REG_M_AXIS_DATA_COUNT_read();
	// printk("%s get_side_info m axis data count %d per trans %d\n", side_ch_compatible_str, num_dma_symbol, num_dma_symbol_per_trans);
	num_dma_symbol = num_dma_symbol_per_trans*(num_dma_symbol/num_dma_symbol_per_trans);
	// printk("%s get_side_info actual num dma symbol %d\n", side_ch_compatible_str, num_dma_symbol);
	if (num_dma_symbol == 0)
		return(-2);

	side_info_buf_size = num_dma_symbol*8;
	side_info_buf_dma = dma_map_single(chan_to_ps_dev->dev, side_info_buf, side_info_buf_size, DMA_DEV_TO_MEM);
	if (dma_mapping_error(chan_to_ps_dev->dev, side_info_buf_dma)) {
		printk("%s get_side_info WARNING chan_to_ps_dev DMA mapping error\n", side_ch_compatible_str);
		return(-3);
	}

	sg_init_table(chan_to_ps_sg, 1);
	sg_dma_address(&chan_to_ps_sg[0]) = side_info_buf_dma;
	sg_dma_len(&chan_to_ps_sg[0]) = side_info_buf_size;

	chan_to_ps_d = chan_to_ps_dev->device_prep_slave_sg(chan_to_ps, chan_to_ps_sg, 1, DMA_DEV_TO_MEM, flags, NULL);
	if (!chan_to_ps_d) {
		printk("%s get_side_info WARNING !chan_to_ps_d\n", side_ch_compatible_str);
		goto err_dst_buf_with_unmap;
	}

	init_completion(&chan_to_ps_cmp);
	chan_to_ps_d->callback = chan_to_ps_callback;
	chan_to_ps_d->callback_param = &chan_to_ps_cmp;

	chan_to_ps_cookie = chan_to_ps_d->tx_submit(chan_to_ps_d);
	if (dma_submit_error(chan_to_ps_cookie)) {
		printk("%s get_side_info WARNING dma_submit_error\n", side_ch_compatible_str);
		goto err_dst_buf_with_unmap;
	}

	SIDE_CH_REG_NUM_DMA_SYMBOL_write(num_dma_symbol); //dma from fpga will start automatically

	dma_async_issue_pending(chan_to_ps);

	chan_to_ps_tmo = wait_for_completion_timeout(&chan_to_ps_cmp, chan_to_ps_tmo);
	status = dma_async_is_tx_complete(chan_to_ps, chan_to_ps_cookie, NULL, NULL);
	if (chan_to_ps_tmo == 0) {
		printk("%s get_side_info WARNING chan_to_ps_tmo == 0\n", side_ch_compatible_str);
		goto err_dst_buf_with_unmap;
	} else if (status != DMA_COMPLETE) {
		printk("%s get_side_info WARNING chan_to_ps status != DMA_COMPLETE\n", side_ch_compatible_str);
		goto err_dst_buf_with_unmap;
	}

	dma_unmap_single(chan_to_ps_dev->dev, side_info_buf_dma, side_info_buf_size, DMA_DEV_TO_MEM);
	return(side_info_buf_size);

err_dst_buf_with_unmap:
	dma_unmap_single(chan_to_ps_dev->dev, side_info_buf_dma, side_info_buf_size, DMA_DEV_TO_MEM);
	return(-100);
}

// -----------------netlink recv and send-----------------
// should align with side_ch_ctl.c in user_space
#define ACTION_INVALID       0
#define ACTION_REG_WRITE     1
#define ACTION_REG_READ      2
#define ACTION_SIDE_INFO_GET 3

#define REG_TYPE_INVALID     0
#define REG_TYPE_HARDWARE    1
#define REG_TYPE_SOFTWARE    2

// #define NETLINK_USER 31
struct sock *nl_sk = NULL;
static void side_ch_nl_recv_msg(struct sk_buff *skb) {
	struct nlmsghdr *nlh;
	int pid;
	struct sk_buff *skb_out;
	int msg_size;
	int *msg=(int*)side_info_buf;
	int action_flag, reg_type, reg_idx;
	u32 reg_val, *cmd_buf;
	int res;

	// printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

	// msg_size=strlen(msg);

	nlh=(struct nlmsghdr*)skb->data;
	cmd_buf = (u32*)nlmsg_data(nlh);
	// printk(KERN_INFO "Netlink received msg payload:%s\n",(char*)nlmsg_data(nlh));
	action_flag = cmd_buf[0];
    reg_type = cmd_buf[1];
    reg_idx = cmd_buf[2];
    reg_val = cmd_buf[3];
	// printk("%s recv msg: len %d action_flag %d reg_type %d reg_idx %d reg_val %u\n", side_ch_compatible_str, nlmsg_len(nlh), action_flag, reg_type, reg_idx, reg_val);

	pid = nlh->nlmsg_pid; /*pid of sending process */

	if (action_flag==ACTION_SIDE_INFO_GET) {
		res = get_side_info(num_eq_init, iq_len_init);
		// printk(KERN_INFO "%s recv msg: get_side_info(%d,%d) res %d\n", side_ch_compatible_str, num_eq_init, iq_len_init, res);
		if (res>0) {
			msg_size = res;
			// printk("%s recv msg: %d %d %d %d %d %d %d %d\n", side_ch_compatible_str, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]);
		} else {
			msg_size = 4;
			msg[0] = -2;
		}
	} else if (action_flag==ACTION_REG_READ) {
		msg_size = 4;
		// if (reg_idx<0 || reg_idx>31) {
		// 	msg[0] = -3;
		// 	printk("%s recv msg: invalid reg_idx\n", side_ch_compatible_str);
		// } else {
			msg[0] = reg_read(reg_idx*4);
		// }
	} else if (action_flag==ACTION_REG_WRITE) {
		msg_size = 4;
		// if (reg_idx<0 || reg_idx>31) {
		// 	msg[0] = -4;
		// 	printk("%s recv msg: invalid reg_idx\n", side_ch_compatible_str);
		// } else {
			msg[0] = 0;
			reg_write(reg_idx*4, reg_val);
		// }
	} else {
		msg_size = 4;
		msg[0] = -1;
		printk("%s recv msg: invalid action_flag\n", side_ch_compatible_str);
	}

	skb_out = nlmsg_new(msg_size,0);
	if(!skb_out)
	{
		printk(KERN_ERR "Failed to allocate new skb\n");
		return;
	} 
	nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);  
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

	memcpy(nlmsg_data(nlh),msg,msg_size);

	res=nlmsg_unicast(nl_sk,skb_out,pid);

	if(res<0)
		printk(KERN_INFO "Error while sending bak to user\n");
}

static int dev_probe(struct platform_device *pdev) {
	struct netlink_kernel_cfg cfg = {
		.input = side_ch_nl_recv_msg,
	};

	struct device_node *np = pdev->dev.of_node;
	struct resource *io;
	int err=1, i;

	printk("\n");

	if (np) {
		const struct of_device_id *match;

		match = of_match_node(dev_of_ids, np);
		if (match) {
			printk("%s dev_probe: match!\n", side_ch_compatible_str);
			err = 0;
		}
	}

	if (err)
		return err;

	/* Request and map I/O memory */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base_addr = devm_ioremap_resource(&pdev->dev, io);
	if (IS_ERR(base_addr))
		return PTR_ERR(base_addr);

	printk("%s dev_probe: io start 0x%p end 0x%p name %s flags 0x%08x desc %s\n", side_ch_compatible_str, (void*)io->start, (void*)io->end, io->name, (u32)io->flags, (char*)io->desc);
	printk("%s dev_probe: base_addr 0x%p\n", side_ch_compatible_str, base_addr);

	printk("%s dev_probe: succeed!\n", side_ch_compatible_str);

	// --------------initialize netlink--------------
	//nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	nl_sk = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
	if(!nl_sk) {
		printk(KERN_ALERT "%s dev_probe: Error creating socket.\n", side_ch_compatible_str);
		return -10;
	}

	//-----------------initialize fpga----------------
	printk("%s dev_probe: num_eq_init %d iq_len_init %d\n",side_ch_compatible_str, num_eq_init, iq_len_init);
	
	// disable potential any action from side channel
	SIDE_CH_REG_MULTI_RST_write(4);
	// SIDE_CH_REG_CONFIG_write(0X6001); // match addr1 and addr2; bit12 FC; bit13 addr1; bit14 addr2
	SIDE_CH_REG_CONFIG_write(0x7001); // the most strict condition to prevent side channel action
	SIDE_CH_REG_IQ_TRIGGER_write(10); // set iq trigger to rssi, which will never happen when rssi_th is 0
	SIDE_CH_REG_NUM_EQ_write(num_eq_init);      // capture CSI + 8*equalizer by default
	if (iq_len_init>0) {//initialize the side channel into iq capture mode
		//Max UDP 65507 bytes; (65507/8)-1 = 8187
		if (iq_len_init>8187) {
			iq_len_init = 8187;
			printk("%s dev_probe: limit iq_len_init to 8187!\n",side_ch_compatible_str);
		}
		SIDE_CH_REG_IQ_CAPTURE_write(1);
		SIDE_CH_REG_PRE_TRIGGER_LEN_write(8190);
		SIDE_CH_REG_IQ_LEN_write(iq_len_init);
		SIDE_CH_REG_IQ_TRIGGER_write(0); // trigger is set to fcs ok/nok (both)
	}

	SIDE_CH_REG_CONFIG_write(0x0001); // allow all packets by default; bit12 FC; bit13 addr1; bit14 addr2

	//rst
	for (i=0;i<8;i++)
		SIDE_CH_REG_MULTI_RST_write(0);
	for (i=0;i<32;i++)
		SIDE_CH_REG_MULTI_RST_write(0xFFFFFFFF);
	for (i=0;i<8;i++)
		SIDE_CH_REG_MULTI_RST_write(0);
	
	// chan_to_pl = dma_request_slave_channel(&(pdev->dev), "rx_dma_mm2s");
	// if (IS_ERR(chan_to_pl)) {
	// 	err = PTR_ERR(chan_to_pl);
	// 	pr_err("%s dev_probe: No channel to PL. %d\n",side_ch_compatible_str,err);
	// 	goto free_chan_to_pl;
	// }

	chan_to_ps = dma_request_chan(&(pdev->dev), "tx_dma_s2mm");
	if (IS_ERR(chan_to_ps) || chan_to_ps==NULL) {
		err = PTR_ERR(chan_to_ps);
		if (err != -EPROBE_DEFER) {
			pr_err("%s dev_probe: No chan_to_ps ret %d chan_to_ps 0x%p\n",side_ch_compatible_str, err, chan_to_ps);
			goto free_chan_to_ps;
		}
	}

	printk("%s dev_probe: DMA channel setup successfully. chan_to_pl 0x%p chan_to_ps 0x%p\n",side_ch_compatible_str, chan_to_pl, chan_to_ps);

	// res = dma_loopback_test(3, 512);
	// printk(KERN_INFO "dma_loopback_test(3, 512) res %d\n", res);

	err = init_side_channel();
	printk("%s dev_probe: init_side_channel() err %d\n",side_ch_compatible_str, err);

	return(err);

	// err = dma_loopback_test(7, 512);
	// if (err == 0)
	// 	return(err);
	// else
	// 	dma_release_channel(chan_to_ps);

free_chan_to_ps:
	err = -2;
	dma_release_channel(chan_to_ps);
	return err;

// free_chan_to_pl:
// 	err = -1;
// 	dma_release_channel(chan_to_pl);
// 	return err;
}

static int dev_remove(struct platform_device *pdev)
{
	printk("\n");

	printk("%s dev_remove: release nl_sk\n", side_ch_compatible_str);
	netlink_kernel_release(nl_sk);

	// pr_info("%s dev_remove: dropped chan_to_pl 0x%p\n", side_ch_compatible_str, chan_to_pl);
	// if (chan_to_pl != NULL) {
	// 	pr_info("%s dev_remove: dropped channel %s\n", side_ch_compatible_str, dma_chan_name(chan_to_pl));
	// 	// dmaengine_terminate_all(chan_to_pl); //this also terminate sdr.ko. do not use
	// 	dma_release_channel(chan_to_pl);
	// }

	pr_info("%s dev_remove: dropped chan_to_ps 0x%p\n", side_ch_compatible_str, chan_to_ps);
	if (chan_to_pl != NULL) {
		pr_info("%s dev_remove: dropped channel %s\n", side_ch_compatible_str, dma_chan_name(chan_to_ps));
		// dmaengine_terminate_all(chan_to_ps); //this also terminate sdr.ko. do not use
		dma_release_channel(chan_to_ps);
	}

	if (side_info_buf != NULL)
		kfree(side_info_buf);
	
	printk("%s dev_remove: base_addr 0x%p\n", side_ch_compatible_str, base_addr);
	printk("%s dev_remove: succeed!\n", side_ch_compatible_str);
	return 0;
}

static struct platform_driver dev_driver = {
	.driver = {
		.name = "sdr,side_ch",
		.owner = THIS_MODULE,
		.of_match_table = dev_of_ids,
	},
	.probe = dev_probe,
	.remove = dev_remove,
};

module_platform_driver(dev_driver);

MODULE_AUTHOR("Xianjun Jiao");
MODULE_DESCRIPTION("sdr,side_ch");
MODULE_LICENSE("GPL v2");
