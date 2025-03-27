// Author: Xianjun Jiao, Michael Mehari, Wei Liu
// SPDX-FileCopyrightText: 2019 UGent
// SPDX-License-Identifier: AGPL-3.0-or-later

// #define TX_INTF_IQ_WRITE_TXT_FORMAT 1//while using TXT format, the txt file size should <= 4096!!!
#define TX_INTF_IQ_WRITE_BIN_FORMAT 1

#ifdef TX_INTF_IQ_WRITE_TXT_FORMAT
static int is_valid_iq_number(int c) {
	if (c==32 || (c>=44 && c<=57))
		return(1);
	else
		return(0);
}

static ssize_t openwifi_tx_intf_bin_iq_write(struct file *filp, struct kobject *kobj,
		       struct bin_attribute *bin_attr,
		       char *buf, loff_t off, size_t count)
{
	struct platform_device *pdev = to_platform_device(kobj_to_dev(kobj));
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	int ret, i, q, num_iq;
	char *line;
	char *ptr = buf;

	printk("%s openwifi_tx_intf_bin_iq_write: count %d\n", sdr_compatible_str, (int)count);

	num_iq = 0;
	line = ptr;
	while (1) {
		ret = sscanf(line, "%d,%d\n", &i, &q);
		if (ret == 0) {
			printk("%s openwifi_tx_intf_bin_iq_write: sscanf ret 0\n", sdr_compatible_str);
			break;
		} else if (ret != 2) {
			printk("%s openwifi_tx_intf_bin_iq_write: sscanf ret %d i %d q %d num_iq %d\n", sdr_compatible_str, ret, i, q, num_iq);
			return -EINVAL;
		}
		
		priv->tx_intf_arbitrary_iq[num_iq] = ( (q<<16)|(i&0xFFFF) );
		num_iq++;
		if (num_iq == 512) {
			printk("%s openwifi_tx_intf_bin_iq_write: num_iq reach 512\n", sdr_compatible_str);
			break;
		}

		//go to the next line
		while(is_valid_iq_number(ptr[0]))
			ptr++;
		while( (is_valid_iq_number(ptr[0])==0)&&(ptr[0]!=0) )
			ptr++;
		if (ptr[0] == 0) {
			printk("%s openwifi_tx_intf_bin_iq_write: ptr[0] == 0\n", sdr_compatible_str);
			break;
		}
		line = ptr;
	}
	priv->tx_intf_arbitrary_iq_num = num_iq;

	printk("%s openwifi_tx_intf_bin_iq_write: num_iq %d\n", sdr_compatible_str, num_iq);
	//print i/q
	for (i=0; i<num_iq; i++)
		printk("%d %d\n", (short)(priv->tx_intf_arbitrary_iq[i]&0xffff), (short)((priv->tx_intf_arbitrary_iq[i]>>16)&0xffff) );
	
	return count;
}
#else
static ssize_t openwifi_tx_intf_bin_iq_write(struct file *filp, struct kobject *kobj,
		       struct bin_attribute *bin_attr,
		       char *buf, loff_t off, size_t count)
{
	struct platform_device *pdev = to_platform_device(kobj_to_dev(kobj));
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	int num_iq, i;

	printk("%s openwifi_tx_intf_bin_iq_write: count %d\n", sdr_compatible_str, (int)count);

	if ((count%4) != 0) {
		printk("%s openwifi_tx_intf_bin_iq_write: count is not integer times of 4!\n", sdr_compatible_str);
		return -EINVAL;
	}

	num_iq = count/4;
	priv->tx_intf_arbitrary_iq_num = num_iq;

	for (i=0; i<num_iq; i++) {
		priv->tx_intf_arbitrary_iq[i] = (*((u32*)(buf+(i*4))));
	}

	// printk("%s openwifi_tx_intf_bin_iq_write: num_iq %d\n", sdr_compatible_str, num_iq);
	// //print i/q
	// for (i=0; i<num_iq; i++)
	// 	printk("%d %d\n", (short)(priv->tx_intf_arbitrary_iq[i]&0xffff), (short)((priv->tx_intf_arbitrary_iq[i]>>16)&0xffff) );
	
	return count;
}
#endif

static ssize_t openwifi_tx_intf_bin_iq_read(struct file *filp, struct kobject *kobj,
		       struct bin_attribute *bin_attr,
		       char *buf, loff_t off, size_t count)
{
	struct platform_device *pdev = to_platform_device(kobj_to_dev(kobj));
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	int num_iq, ret_size, i;

	if (off)
		return 0;

	num_iq = priv->tx_intf_arbitrary_iq_num;

	// printk("%s openwifi_tx_intf_bin_iq_read: num_iq %d\n", sdr_compatible_str, num_iq);
	// //print i/q
	// for (i=0; i<num_iq; i++)
	// 	printk("%d %d\n", (short)(priv->tx_intf_arbitrary_iq[i]&0xffff), (short)((priv->tx_intf_arbitrary_iq[i]>>16)&0xffff) );

	ret_size = sprintf(buf, "%d\n", num_iq);
	if (num_iq==0 || num_iq>512) {
		ret_size = ret_size + sprintf(buf+ret_size, "num_iq is wrong!\n");
		return ret_size;
	}

	//print i
	for (i=0; i<num_iq; i++)
		ret_size = ret_size + sprintf(buf+ret_size, "%d ", (short)(priv->tx_intf_arbitrary_iq[i]&0xffff) );
	ret_size = ret_size + sprintf(buf+ret_size, "\n");
	
	//print q
	for (i=0; i<num_iq; i++)
		ret_size = ret_size + sprintf(buf+ret_size, "%d ", (short)((priv->tx_intf_arbitrary_iq[i]>>16)&0xffff) );
	ret_size = ret_size + sprintf(buf+ret_size, "\n");

	return ret_size;
}

static ssize_t tx_intf_iq_ctl_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->tx_intf_iq_ctl);
}
static ssize_t tx_intf_iq_ctl_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	int i;
	int ret = kstrtol(buf, 10, &readin);

	priv->tx_intf_iq_ctl = readin;

	tx_intf_api->TX_INTF_REG_ARBITRARY_IQ_CTL_write(1); // switch to iq mode
	printk("%s tx_intf_iq_ctl_store: Will send %d I/Q\n", sdr_compatible_str, priv->tx_intf_arbitrary_iq_num);
	for (i=0; i<priv->tx_intf_arbitrary_iq_num; i++) {
		tx_intf_api->TX_INTF_REG_ARBITRARY_IQ_write(priv->tx_intf_arbitrary_iq[i]);
	}
	tx_intf_api->TX_INTF_REG_ARBITRARY_IQ_CTL_write(3); // start send
	tx_intf_api->TX_INTF_REG_ARBITRARY_IQ_CTL_write(0);

	return ret ? ret : len;
}

static DEVICE_ATTR(tx_intf_iq_ctl, S_IRUGO | S_IWUSR, tx_intf_iq_ctl_show, tx_intf_iq_ctl_store);
static struct attribute *tx_intf_attributes[] = {
	&dev_attr_tx_intf_iq_ctl.attr,
	NULL,
};
static const struct attribute_group tx_intf_attribute_group = {
	.attrs = tx_intf_attributes,
};

static ssize_t stat_enable_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.stat_enable);
}
static ssize_t stat_enable_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.stat_enable = readin;

	return ret ? ret : len;
}

static ssize_t tx_prio_queue_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	u32 i, ret_size = 0;

	for (i=0; i<MAX_NUM_SW_QUEUE; i++) {
		ret_size = ret_size + sprintf(buf+ret_size, "%u %u %u %u %u %u %u %u %u %u %u %u\n", 
		priv->stat.tx_prio_num[i],
		priv->stat.tx_prio_interrupt_num[i], 
		priv->stat.tx_prio_stop0_fake_num[i], 
		priv->stat.tx_prio_stop0_real_num[i],
		priv->stat.tx_prio_stop1_num[i],
		priv->stat.tx_prio_wakeup_num[i],
		priv->stat.tx_queue_num[i],
		priv->stat.tx_queue_interrupt_num[i],
		priv->stat.tx_queue_stop0_fake_num[i],
		priv->stat.tx_queue_stop0_real_num[i],
		priv->stat.tx_queue_stop1_num[i],
		priv->stat.tx_queue_wakeup_num[i]);
	}
	
	return ret_size;
}
static ssize_t tx_prio_queue_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	u32 i;
	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	for (i=0; i<MAX_NUM_SW_QUEUE; i++) {
		priv->stat.tx_prio_num[i] = 0;
		priv->stat.tx_prio_interrupt_num[i] = 0;
		priv->stat.tx_prio_stop0_fake_num[i] = 0;
		priv->stat.tx_prio_stop0_real_num[i] = 0;
		priv->stat.tx_prio_stop1_num[i] = 0;
		priv->stat.tx_prio_wakeup_num[i] = 0;
	}
	for (i=0; i<MAX_NUM_HW_QUEUE; i++) {
		priv->stat.tx_queue_num[i] = 0;
		priv->stat.tx_queue_interrupt_num[i] = 0;
		priv->stat.tx_queue_stop0_fake_num[i] = 0;
		priv->stat.tx_queue_stop0_real_num[i] = 0;
		priv->stat.tx_queue_stop1_num[i] = 0;
		priv->stat.tx_queue_wakeup_num[i] = 0;
	}

	return ret ? ret : len;
}

static ssize_t tx_data_pkt_need_ack_num_total_fail_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.tx_data_pkt_need_ack_num_total_fail);
}
static ssize_t tx_data_pkt_need_ack_num_total_fail_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.tx_data_pkt_need_ack_num_total_fail = readin;

	return ret ? ret : len;
}

static ssize_t tx_data_pkt_need_ack_num_total_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.tx_data_pkt_need_ack_num_total);
}
static ssize_t tx_data_pkt_need_ack_num_total_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.tx_data_pkt_need_ack_num_total = readin;

	return ret ? ret : len;
}

static ssize_t tx_mgmt_pkt_need_ack_num_total_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.tx_mgmt_pkt_need_ack_num_total);
}
static ssize_t tx_mgmt_pkt_need_ack_num_total_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.tx_mgmt_pkt_need_ack_num_total = readin;

	return ret ? ret : len;
}

static ssize_t tx_mgmt_pkt_need_ack_num_total_fail_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.tx_mgmt_pkt_need_ack_num_total_fail);
}
static ssize_t tx_mgmt_pkt_need_ack_num_total_fail_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.tx_mgmt_pkt_need_ack_num_total_fail = readin;

	return ret ? ret : len;
}

static ssize_t tx_data_pkt_need_ack_num_retx_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u %u %u %u %u %u\n", 
					priv->stat.tx_data_pkt_need_ack_num_retx[0], 
					priv->stat.tx_data_pkt_need_ack_num_retx[1], 
					priv->stat.tx_data_pkt_need_ack_num_retx[2], 
					priv->stat.tx_data_pkt_need_ack_num_retx[3], 
					priv->stat.tx_data_pkt_need_ack_num_retx[4], 
					priv->stat.tx_data_pkt_need_ack_num_retx[5]);
}
static ssize_t tx_data_pkt_need_ack_num_retx_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin), i;

	for (i=0; i<6; i++)
		priv->stat.tx_data_pkt_need_ack_num_retx[i] = readin;

	return ret ? ret : len;
}

static ssize_t tx_mgmt_pkt_need_ack_num_retx_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u %u %u\n", 
				priv->stat.tx_mgmt_pkt_need_ack_num_retx[0],
				priv->stat.tx_mgmt_pkt_need_ack_num_retx[1],
				priv->stat.tx_mgmt_pkt_need_ack_num_retx[2]);
}
static ssize_t tx_mgmt_pkt_need_ack_num_retx_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin), i;

	for (i=0; i<3; i++)
		priv->stat.tx_mgmt_pkt_need_ack_num_retx[i] = readin;

	return ret ? ret : len;
}

static ssize_t tx_data_pkt_need_ack_num_retx_fail_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u %u %u %u %u %u\n", 
				priv->stat.tx_data_pkt_need_ack_num_retx_fail[0],
				priv->stat.tx_data_pkt_need_ack_num_retx_fail[1],
				priv->stat.tx_data_pkt_need_ack_num_retx_fail[2],
				priv->stat.tx_data_pkt_need_ack_num_retx_fail[3],
				priv->stat.tx_data_pkt_need_ack_num_retx_fail[4],
				priv->stat.tx_data_pkt_need_ack_num_retx_fail[5]);
}
static ssize_t tx_data_pkt_need_ack_num_retx_fail_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin), i;

	for (i=0; i<6; i++)
		priv->stat.tx_data_pkt_need_ack_num_retx_fail[i] = readin;

	return ret ? ret : len;
}

static ssize_t tx_mgmt_pkt_need_ack_num_retx_fail_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u %u %u\n", 
				priv->stat.tx_mgmt_pkt_need_ack_num_retx_fail[0], 
				priv->stat.tx_mgmt_pkt_need_ack_num_retx_fail[1], 
				priv->stat.tx_mgmt_pkt_need_ack_num_retx_fail[2]);
}
static ssize_t tx_mgmt_pkt_need_ack_num_retx_fail_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin), i;

	for (i=0; i<3; i++)
		priv->stat.tx_mgmt_pkt_need_ack_num_retx_fail[i] = readin;

	return ret ? ret : len;
}

static ssize_t tx_data_pkt_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	bool use_ht_rate = ((priv->stat.tx_data_pkt_mcs_realtime&0x80000000)!=0);
	u32 rate_hw_value = (priv->stat.tx_data_pkt_mcs_realtime&0x7fffffff);
	
	return sprintf(buf, "%uM\n", (use_ht_rate == false ? wifi_rate_all[rate_hw_value] : wifi_rate_all[rate_hw_value + 12]));
}
static ssize_t tx_data_pkt_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.tx_data_pkt_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t tx_mgmt_pkt_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	bool use_ht_rate = ((priv->stat.tx_mgmt_pkt_mcs_realtime&0x80000000)!=0);
	u32 rate_hw_value = (priv->stat.tx_mgmt_pkt_mcs_realtime&0x7fffffff);
	
	return sprintf(buf, "%uM\n", (use_ht_rate == false ? wifi_rate_all[rate_hw_value] : wifi_rate_all[rate_hw_value + 12]));
}
static ssize_t tx_mgmt_pkt_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.tx_mgmt_pkt_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t tx_mgmt_pkt_fail_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	bool use_ht_rate = ((priv->stat.tx_mgmt_pkt_fail_mcs_realtime&0x80000000)!=0);
	u32 rate_hw_value = (priv->stat.tx_mgmt_pkt_fail_mcs_realtime&0x7fffffff);
	
	return sprintf(buf, "%uM\n", (use_ht_rate == false ? wifi_rate_all[rate_hw_value] : wifi_rate_all[rate_hw_value + 12]));
}
static ssize_t tx_mgmt_pkt_fail_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.tx_mgmt_pkt_fail_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t tx_data_pkt_fail_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	bool use_ht_rate = ((priv->stat.tx_data_pkt_fail_mcs_realtime&0x80000000)!=0);
	u32 rate_hw_value = (priv->stat.tx_data_pkt_fail_mcs_realtime&0x7fffffff);
	
	return sprintf(buf, "%uM\n", (use_ht_rate == false ? wifi_rate_all[rate_hw_value] : wifi_rate_all[rate_hw_value + 12]));
}
static ssize_t tx_data_pkt_fail_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.tx_data_pkt_fail_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_target_sender_mac_addr_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%08x\n", reverse32(priv->stat.rx_target_sender_mac_addr));
}
static ssize_t rx_target_sender_mac_addr_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	u32 readin;
	u32 ret = kstrtouint(buf, 16, &readin);

	priv->stat.rx_target_sender_mac_addr = reverse32(readin);

	return ret ? ret : len;
}

static ssize_t rx_data_ok_agc_gain_value_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_data_ok_agc_gain_value_realtime);
}
static ssize_t rx_data_ok_agc_gain_value_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_data_ok_agc_gain_value_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_mgmt_ok_agc_gain_value_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_mgmt_ok_agc_gain_value_realtime);
}
static ssize_t rx_mgmt_ok_agc_gain_value_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_mgmt_ok_agc_gain_value_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_data_fail_agc_gain_value_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_data_fail_agc_gain_value_realtime);
}
static ssize_t rx_data_fail_agc_gain_value_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_data_fail_agc_gain_value_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_mgmt_fail_agc_gain_value_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_mgmt_fail_agc_gain_value_realtime);
}
static ssize_t rx_mgmt_fail_agc_gain_value_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_mgmt_fail_agc_gain_value_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_ack_ok_agc_gain_value_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_ack_ok_agc_gain_value_realtime);
}
static ssize_t rx_ack_ok_agc_gain_value_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_ack_ok_agc_gain_value_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_monitor_all_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_monitor_all);
}
static ssize_t rx_monitor_all_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);
	u32 filter_flag;

	priv->stat.rx_monitor_all = readin;

	filter_flag = xpu_api->XPU_REG_FILTER_FLAG_read();
	if (readin>0) {// set to fpga
		filter_flag = (filter_flag|MONITOR_ALL);
	} else {
		filter_flag = (filter_flag&(~MONITOR_ALL));
	}
	xpu_api->XPU_REG_FILTER_FLAG_write(filter_flag);

	return ret ? ret : len;
}

static ssize_t rx_data_pkt_num_total_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_data_pkt_num_total);
}
static ssize_t rx_data_pkt_num_total_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_data_pkt_num_total = readin;

	return ret ? ret : len;
}

static ssize_t rx_data_pkt_num_fail_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_data_pkt_num_fail);
}
static ssize_t rx_data_pkt_num_fail_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_data_pkt_num_fail = readin;

	return ret ? ret : len;
}

static ssize_t rx_mgmt_pkt_num_total_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_mgmt_pkt_num_total);
}
static ssize_t rx_mgmt_pkt_num_total_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_mgmt_pkt_num_total = readin;

	return ret ? ret : len;
}

static ssize_t rx_mgmt_pkt_num_fail_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_mgmt_pkt_num_fail);
}
static ssize_t rx_mgmt_pkt_num_fail_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_mgmt_pkt_num_fail = readin;

	return ret ? ret : len;
}

static ssize_t rx_ack_pkt_num_total_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_ack_pkt_num_total);
}
static ssize_t rx_ack_pkt_num_total_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_ack_pkt_num_total = readin;

	return ret ? ret : len;
}

static ssize_t rx_ack_pkt_num_fail_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.rx_ack_pkt_num_fail);
}
static ssize_t rx_ack_pkt_num_fail_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_ack_pkt_num_fail = readin;

	return ret ? ret : len;
}

static ssize_t rx_data_pkt_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	return sprintf(buf, "%uM\n", wifi_rate_table[priv->stat.rx_data_pkt_mcs_realtime]);
}
static ssize_t rx_data_pkt_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_data_pkt_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_data_pkt_fail_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	return sprintf(buf, "%uM\n", wifi_rate_table[priv->stat.rx_data_pkt_fail_mcs_realtime]);
}
static ssize_t rx_data_pkt_fail_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_data_pkt_fail_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_mgmt_pkt_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	return sprintf(buf, "%uM\n", wifi_rate_table[priv->stat.rx_mgmt_pkt_mcs_realtime]);
}
static ssize_t rx_mgmt_pkt_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_mgmt_pkt_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_mgmt_pkt_fail_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	return sprintf(buf, "%uM\n", wifi_rate_table[priv->stat.rx_mgmt_pkt_fail_mcs_realtime]);
}
static ssize_t rx_mgmt_pkt_fail_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_mgmt_pkt_fail_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t rx_ack_pkt_mcs_realtime_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	return sprintf(buf, "%uM\n", wifi_rate_table[priv->stat.rx_ack_pkt_mcs_realtime]);
}
static ssize_t rx_ack_pkt_mcs_realtime_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.rx_ack_pkt_mcs_realtime = readin;

	return ret ? ret : len;
}

static ssize_t restrict_freq_mhz_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.restrict_freq_mhz);
}
static ssize_t restrict_freq_mhz_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

  static struct ieee80211_conf channel_conf_tmp;
  static struct ieee80211_channel channel_tmp;

  long readin;
	u32 ret = kstrtol(buf, 10, &readin);

  channel_conf_tmp.chandef.chan = (&channel_tmp);

	priv->stat.restrict_freq_mhz = readin;

  channel_conf_tmp.chandef.chan->center_freq = priv->stat.restrict_freq_mhz;
  ad9361_rf_set_channel(dev, &channel_conf_tmp);

	return ret ? ret : len;
}

static ssize_t csma_cfg0_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	u32 reg_val;

	reg_val = xpu_api->XPU_REG_FORCE_IDLE_MISC_read();
	priv->stat.csma_cfg0 = reg_val;
	
	return sprintf(buf, "nav_disable %d difs_disable %d eifs_disable %d eifs_by_rx_fail_disable %d eifs_by_tx_fail_disable %d cw_override %d cw override val %d wait_after_decode_top %d\n", 
					(reg_val>>31)&1, 
					(reg_val>>30)&1, 
					(reg_val>>29)&1, 
          (reg_val>>27)&1, 
          (reg_val>>26)&1, 
					(reg_val>>28)&1, 
					(reg_val>>16)&0xf, 
					(reg_val>>0)&0xff);
}
static ssize_t csma_cfg0_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	u32 disable_flag, idx_from_msb, reg_val;
	u32 readin;
	u32 ret = kstrtouint(buf, 16, &readin);

	disable_flag =  (readin&0xf);
	idx_from_msb = ((readin>>4)&0xf);

	reg_val = xpu_api->XPU_REG_FORCE_IDLE_MISC_read();

	if (disable_flag)
		reg_val = (reg_val|(1<<(31-idx_from_msb)));
	else
		reg_val = (reg_val&(~(1<<(31-idx_from_msb))));

	xpu_api->XPU_REG_FORCE_IDLE_MISC_write(reg_val);

	priv->stat.csma_cfg0 = reg_val;

	return ret ? ret : len;
}

static ssize_t cw_max_min_cfg_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	u32 ret_size, reg_val;

	reg_val = xpu_api->XPU_REG_CSMA_CFG_read();

	ret_size = sprintf(buf, "FPGA  cw max min for q3 to q0: %d %d; %d %d; %d %d; %d %d\n",
						(1<<((reg_val>>28)&0xF))-1, 
						(1<<((reg_val>>24)&0xF))-1, 
						(1<<((reg_val>>20)&0xF))-1, 
						(1<<((reg_val>>16)&0xF))-1, 
						(1<<((reg_val>>12)&0xF))-1, 
						(1<<((reg_val>> 8)&0xF))-1, 
						(1<<((reg_val>> 4)&0xF))-1, 
						(1<<((reg_val>> 0)&0xF))-1);
	ret_size = ret_size + sprintf(buf+ret_size, "FPGA  cw max min for q3 to q0: %08x\n",reg_val);

	if (priv->stat.cw_max_min_cfg) {
		reg_val = priv->stat.cw_max_min_cfg;
		ret_size = ret_size + sprintf(buf+ret_size, "SYSFS cw max min for q3 to q0: %d %d; %d %d; %d %d; %d %d\n",
						(1<<((reg_val>>28)&0xF))-1, 
						(1<<((reg_val>>24)&0xF))-1, 
						(1<<((reg_val>>20)&0xF))-1, 
						(1<<((reg_val>>16)&0xF))-1, 
						(1<<((reg_val>>12)&0xF))-1, 
						(1<<((reg_val>> 8)&0xF))-1, 
						(1<<((reg_val>> 4)&0xF))-1, 
						(1<<((reg_val>> 0)&0xF))-1);
		ret_size = ret_size + sprintf(buf+ret_size, "SYSFS cw max min for q3 to q0: %08x\n",reg_val);
	}

	return ret_size;
}

static ssize_t cw_max_min_cfg_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	u32 readin;
	u32 ret = kstrtouint(buf, 16, &readin);

	// printk("%s %d\n", buf, readin);

	priv->stat.cw_max_min_cfg = readin;
	if (readin)
		xpu_api->XPU_REG_CSMA_CFG_write(readin);

	return ret ? ret : len;
}

static ssize_t dbg_ch0_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.dbg_ch0);
}
static ssize_t dbg_ch0_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.dbg_ch0 = readin;

	// xpu_api->XPU_REG_DIFS_ADVANCE_write((readin<<16)|2); //us. bit31~16 max pkt length threshold
	// rx_intf_api->RX_INTF_REG_START_TRANS_TO_PS_write(readin<<16); //bit31~16 max pkt length threshold
	// openofdm_rx_api->OPENOFDM_RX_REG_SOFT_DECODING_write((readin<<16)|1); //bit1 enable soft decoding; bit31~16 max pkt length threshold

	return ret ? ret : len;
}

static ssize_t dbg_ch1_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.dbg_ch1);
}
static ssize_t dbg_ch1_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.dbg_ch1 = readin;

	return ret ? ret : len;
}

static ssize_t dbg_ch2_show(struct device *input_dev, struct device_attribute *attr, char *buf) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;
	
	return sprintf(buf, "%u\n", priv->stat.dbg_ch2);
}
static ssize_t dbg_ch2_store(struct device *input_dev, struct device_attribute *attr, const char *buf, size_t len) 
{
	struct platform_device *pdev = to_platform_device(input_dev);
	struct ieee80211_hw *dev = platform_get_drvdata(pdev);
	struct openwifi_priv *priv = dev->priv;

	long readin;
	u32 ret = kstrtol(buf, 10, &readin);

	priv->stat.dbg_ch2 = readin;

	return ret ? ret : len;
}

static DEVICE_ATTR(stat_enable, S_IRUGO | S_IWUSR, stat_enable_show, stat_enable_store);
static DEVICE_ATTR(tx_prio_queue, S_IRUGO | S_IWUSR, tx_prio_queue_show, tx_prio_queue_store);
static DEVICE_ATTR(tx_data_pkt_need_ack_num_total, S_IRUGO | S_IWUSR, tx_data_pkt_need_ack_num_total_show, tx_data_pkt_need_ack_num_total_store);
static DEVICE_ATTR(tx_data_pkt_need_ack_num_total_fail, S_IRUGO | S_IWUSR, tx_data_pkt_need_ack_num_total_fail_show, tx_data_pkt_need_ack_num_total_fail_store);
static DEVICE_ATTR(tx_data_pkt_need_ack_num_retx, S_IRUGO | S_IWUSR, tx_data_pkt_need_ack_num_retx_show, tx_data_pkt_need_ack_num_retx_store);
static DEVICE_ATTR(tx_data_pkt_need_ack_num_retx_fail, S_IRUGO | S_IWUSR, tx_data_pkt_need_ack_num_retx_fail_show, tx_data_pkt_need_ack_num_retx_fail_store);

static DEVICE_ATTR(tx_data_pkt_mcs_realtime, S_IRUGO | S_IWUSR, tx_data_pkt_mcs_realtime_show, tx_data_pkt_mcs_realtime_store);
static DEVICE_ATTR(tx_data_pkt_fail_mcs_realtime, S_IRUGO | S_IWUSR, tx_data_pkt_fail_mcs_realtime_show, tx_data_pkt_fail_mcs_realtime_store);

static DEVICE_ATTR(tx_mgmt_pkt_need_ack_num_total, S_IRUGO | S_IWUSR, tx_mgmt_pkt_need_ack_num_total_show, tx_mgmt_pkt_need_ack_num_total_store);
static DEVICE_ATTR(tx_mgmt_pkt_need_ack_num_total_fail, S_IRUGO | S_IWUSR, tx_mgmt_pkt_need_ack_num_total_fail_show, tx_mgmt_pkt_need_ack_num_total_fail_store);
static DEVICE_ATTR(tx_mgmt_pkt_need_ack_num_retx, S_IRUGO | S_IWUSR, tx_mgmt_pkt_need_ack_num_retx_show, tx_mgmt_pkt_need_ack_num_retx_store);
static DEVICE_ATTR(tx_mgmt_pkt_need_ack_num_retx_fail, S_IRUGO | S_IWUSR, tx_mgmt_pkt_need_ack_num_retx_fail_show, tx_mgmt_pkt_need_ack_num_retx_fail_store);

static DEVICE_ATTR(tx_mgmt_pkt_mcs_realtime, S_IRUGO | S_IWUSR, tx_mgmt_pkt_mcs_realtime_show, tx_mgmt_pkt_mcs_realtime_store);
static DEVICE_ATTR(tx_mgmt_pkt_fail_mcs_realtime, S_IRUGO | S_IWUSR, tx_mgmt_pkt_fail_mcs_realtime_show, tx_mgmt_pkt_fail_mcs_realtime_store);

static DEVICE_ATTR(rx_target_sender_mac_addr, S_IRUGO | S_IWUSR, rx_target_sender_mac_addr_show, rx_target_sender_mac_addr_store);
static DEVICE_ATTR(rx_data_ok_agc_gain_value_realtime, S_IRUGO | S_IWUSR, rx_data_ok_agc_gain_value_realtime_show, rx_data_ok_agc_gain_value_realtime_store);
static DEVICE_ATTR(rx_data_fail_agc_gain_value_realtime, S_IRUGO | S_IWUSR, rx_data_fail_agc_gain_value_realtime_show, rx_data_fail_agc_gain_value_realtime_store);
static DEVICE_ATTR(rx_mgmt_ok_agc_gain_value_realtime, S_IRUGO | S_IWUSR, rx_mgmt_ok_agc_gain_value_realtime_show, rx_mgmt_ok_agc_gain_value_realtime_store);
static DEVICE_ATTR(rx_mgmt_fail_agc_gain_value_realtime, S_IRUGO | S_IWUSR, rx_mgmt_fail_agc_gain_value_realtime_show, rx_mgmt_fail_agc_gain_value_realtime_store);
static DEVICE_ATTR(rx_ack_ok_agc_gain_value_realtime, S_IRUGO | S_IWUSR, rx_ack_ok_agc_gain_value_realtime_show, rx_ack_ok_agc_gain_value_realtime_store);

static DEVICE_ATTR(rx_monitor_all, S_IRUGO | S_IWUSR, rx_monitor_all_show, rx_monitor_all_store);
static DEVICE_ATTR(rx_data_pkt_num_total, S_IRUGO | S_IWUSR, rx_data_pkt_num_total_show, rx_data_pkt_num_total_store);
static DEVICE_ATTR(rx_data_pkt_num_fail, S_IRUGO | S_IWUSR, rx_data_pkt_num_fail_show, rx_data_pkt_num_fail_store);
static DEVICE_ATTR(rx_mgmt_pkt_num_total, S_IRUGO | S_IWUSR, rx_mgmt_pkt_num_total_show, rx_mgmt_pkt_num_total_store);
static DEVICE_ATTR(rx_mgmt_pkt_num_fail, S_IRUGO | S_IWUSR, rx_mgmt_pkt_num_fail_show, rx_mgmt_pkt_num_fail_store);
static DEVICE_ATTR(rx_ack_pkt_num_total, S_IRUGO | S_IWUSR, rx_ack_pkt_num_total_show, rx_ack_pkt_num_total_store);
static DEVICE_ATTR(rx_ack_pkt_num_fail, S_IRUGO | S_IWUSR, rx_ack_pkt_num_fail_show, rx_ack_pkt_num_fail_store);

static DEVICE_ATTR(rx_data_pkt_mcs_realtime, S_IRUGO | S_IWUSR, rx_data_pkt_mcs_realtime_show, rx_data_pkt_mcs_realtime_store);
static DEVICE_ATTR(rx_data_pkt_fail_mcs_realtime, S_IRUGO | S_IWUSR, rx_data_pkt_fail_mcs_realtime_show, rx_data_pkt_fail_mcs_realtime_store);
static DEVICE_ATTR(rx_mgmt_pkt_mcs_realtime, S_IRUGO | S_IWUSR, rx_mgmt_pkt_mcs_realtime_show, rx_mgmt_pkt_mcs_realtime_store);
static DEVICE_ATTR(rx_mgmt_pkt_fail_mcs_realtime, S_IRUGO | S_IWUSR, rx_mgmt_pkt_fail_mcs_realtime_show, rx_mgmt_pkt_fail_mcs_realtime_store);
static DEVICE_ATTR(rx_ack_pkt_mcs_realtime, S_IRUGO | S_IWUSR, rx_ack_pkt_mcs_realtime_show, rx_ack_pkt_mcs_realtime_store);

static DEVICE_ATTR(restrict_freq_mhz, S_IRUGO | S_IWUSR, restrict_freq_mhz_show, restrict_freq_mhz_store);

static DEVICE_ATTR(csma_cfg0, S_IRUGO | S_IWUSR, csma_cfg0_show, csma_cfg0_store);
static DEVICE_ATTR(cw_max_min_cfg, S_IRUGO | S_IWUSR, cw_max_min_cfg_show, cw_max_min_cfg_store);

static DEVICE_ATTR(dbg_ch0, S_IRUGO | S_IWUSR, dbg_ch0_show, dbg_ch0_store);
static DEVICE_ATTR(dbg_ch1, S_IRUGO | S_IWUSR, dbg_ch1_show, dbg_ch1_store);
static DEVICE_ATTR(dbg_ch2, S_IRUGO | S_IWUSR, dbg_ch2_show, dbg_ch2_store);

static struct attribute *stat_attributes[] = {
	&dev_attr_stat_enable.attr,

	&dev_attr_tx_prio_queue.attr,

	&dev_attr_tx_data_pkt_need_ack_num_total.attr,
	&dev_attr_tx_data_pkt_need_ack_num_total_fail.attr,

	&dev_attr_tx_data_pkt_need_ack_num_retx.attr,
	&dev_attr_tx_data_pkt_need_ack_num_retx_fail.attr,

	&dev_attr_tx_data_pkt_mcs_realtime.attr,
	&dev_attr_tx_data_pkt_fail_mcs_realtime.attr,

	&dev_attr_tx_mgmt_pkt_need_ack_num_total.attr,
	&dev_attr_tx_mgmt_pkt_need_ack_num_total_fail.attr,

	&dev_attr_tx_mgmt_pkt_need_ack_num_retx.attr,
	&dev_attr_tx_mgmt_pkt_need_ack_num_retx_fail.attr,

	&dev_attr_tx_mgmt_pkt_mcs_realtime.attr,
	&dev_attr_tx_mgmt_pkt_fail_mcs_realtime.attr,

	&dev_attr_rx_target_sender_mac_addr.attr,
	&dev_attr_rx_data_ok_agc_gain_value_realtime.attr,
	&dev_attr_rx_data_fail_agc_gain_value_realtime.attr,
	&dev_attr_rx_mgmt_ok_agc_gain_value_realtime.attr,
	&dev_attr_rx_mgmt_fail_agc_gain_value_realtime.attr,
	&dev_attr_rx_ack_ok_agc_gain_value_realtime.attr,

	&dev_attr_rx_monitor_all.attr,
	&dev_attr_rx_data_pkt_num_total.attr,
	&dev_attr_rx_data_pkt_num_fail.attr,
	&dev_attr_rx_mgmt_pkt_num_total.attr,
	&dev_attr_rx_mgmt_pkt_num_fail.attr,
	&dev_attr_rx_ack_pkt_num_total.attr,
	&dev_attr_rx_ack_pkt_num_fail.attr,

	&dev_attr_rx_data_pkt_mcs_realtime.attr,
	&dev_attr_rx_data_pkt_fail_mcs_realtime.attr,
	&dev_attr_rx_mgmt_pkt_mcs_realtime.attr,
	&dev_attr_rx_mgmt_pkt_fail_mcs_realtime.attr,
	&dev_attr_rx_ack_pkt_mcs_realtime.attr,

	&dev_attr_restrict_freq_mhz.attr,

	&dev_attr_csma_cfg0.attr,
	&dev_attr_cw_max_min_cfg.attr,

	&dev_attr_dbg_ch0.attr,
	&dev_attr_dbg_ch1.attr,
	&dev_attr_dbg_ch2.attr,

	NULL,
};
static const struct attribute_group stat_attribute_group = {
	.attrs = stat_attributes,
};
