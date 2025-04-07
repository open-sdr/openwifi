// Author: Xianjun Jiao, Michael Mehari, Wei Liu
// SPDX-FileCopyrightText: 2019 UGent
// SPDX-License-Identifier: AGPL-3.0-or-later

static int openwifi_testmode_cmd(struct ieee80211_hw *hw, struct ieee80211_vif *vif, void *data, int len)
{
  static struct ieee80211_conf channel_conf_tmp;
  static struct ieee80211_channel channel_tmp;
  struct openwifi_priv *priv = hw->priv;
  struct nlattr *tb[OPENWIFI_ATTR_MAX + 1];
  struct sk_buff *skb;
  int err;
  u32 tmp=-1, reg_cat, reg_addr, reg_val, reg_addr_idx, tsft_high, tsft_low;
  int tmp_int;

  err = nla_parse(tb, OPENWIFI_ATTR_MAX, data, len, openwifi_testmode_policy, NULL);
  if (err)
    return err;

  if (!tb[OPENWIFI_ATTR_CMD])
    return -EINVAL;

  channel_conf_tmp.chandef.chan = (&channel_tmp);

  switch (nla_get_u32(tb[OPENWIFI_ATTR_CMD])) {
  case OPENWIFI_CMD_SET_GAP:
    if (!tb[OPENWIFI_ATTR_GAP])
      return -EINVAL;
    tmp = nla_get_u32(tb[OPENWIFI_ATTR_GAP]);
    printk("%s XPU_REG_CSMA_CFG_write %08x (Check openwifi_conf_tx() in sdr.c to understand)\n", sdr_compatible_str, tmp);
    xpu_api->XPU_REG_CSMA_CFG_write(tmp); // unit us
    return 0;
  case OPENWIFI_CMD_GET_GAP:
    skb = (struct sk_buff *)cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
    if (!skb)
      return -ENOMEM;
    tmp = xpu_api->XPU_REG_CSMA_CFG_read();
    if (nla_put_u32(skb, OPENWIFI_ATTR_GAP, tmp))
      goto nla_put_failure;
    return cfg80211_testmode_reply(skb);
  case OPENWIFI_CMD_SET_SLICE_IDX:
    if (!tb[OPENWIFI_ATTR_SLICE_IDX])
      return -EINVAL;
    tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_IDX]);
    printk("%s set openwifi slice_idx in hex: %08x\n", sdr_compatible_str, tmp);
    if (tmp == MAX_NUM_HW_QUEUE) {
      printk("%s set openwifi slice_idx reset all queue counter.\n", sdr_compatible_str);
      xpu_api->XPU_REG_MULTI_RST_write(1<<7); //bit7 reset the counter for all queues at the same time
      xpu_api->XPU_REG_MULTI_RST_write(0<<7); 
    } else {
      priv->slice_idx = tmp;
    }
    return 0;
  case OPENWIFI_CMD_GET_SLICE_IDX:
    skb = (struct sk_buff *)cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
    if (!skb)
      return -ENOMEM;
    tmp = priv->slice_idx;
    if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_IDX, tmp))
      goto nla_put_failure;
    printk("%s get openwifi slice_idx in hex: %08x\n", sdr_compatible_str, tmp);
    return cfg80211_testmode_reply(skb);
  case OPENWIFI_CMD_SET_ADDR:
    if (!tb[OPENWIFI_ATTR_ADDR])
      return -EINVAL;
    tmp = nla_get_u32(tb[OPENWIFI_ATTR_ADDR]);
    if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
      printk("%s set openwifi slice_target_mac_addr(low32) WARNING: current slice idx %d is invalid!\n", sdr_compatible_str, priv->slice_idx);
      return -EOPNOTSUPP;
    } else {
      printk("%s set openwifi slice_target_mac_addr(low32) in hex: %08x to slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
      priv->dest_mac_addr_queue_map[priv->slice_idx] = reverse32(tmp);
    }
    return 0;
  case OPENWIFI_CMD_GET_ADDR:
    skb = (struct sk_buff *)cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
    if (!skb)
      return -ENOMEM;
    if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
      tmp = -1;
    } else {
      tmp = reverse32(priv->dest_mac_addr_queue_map[priv->slice_idx]);
    }
    if (nla_put_u32(skb, OPENWIFI_ATTR_ADDR, tmp))
      goto nla_put_failure;
    printk("%s get openwifi slice_target_mac_addr(low32) in hex: %08x of slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
    return cfg80211_testmode_reply(skb);

  case OPENWIFI_CMD_SET_SLICE_TOTAL:
    if (!tb[OPENWIFI_ATTR_SLICE_TOTAL])
      return -EINVAL;
    tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_TOTAL]);
    if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
      printk("%s set SLICE_TOTAL(duration) WARNING: current slice idx %d is invalid!\n", sdr_compatible_str, priv->slice_idx);
      return -EOPNOTSUPP;
    } else {
      printk("%s set SLICE_TOTAL(duration) %d usec to slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
      xpu_api->XPU_REG_SLICE_COUNT_TOTAL_write((priv->slice_idx<<20)|tmp);
    }
    return 0;
  case OPENWIFI_CMD_GET_SLICE_TOTAL:
    skb = (struct sk_buff *)cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
    if (!skb)
      return -ENOMEM;
    tmp = (xpu_api->XPU_REG_SLICE_COUNT_TOTAL_read());
    printk("%s get SLICE_TOTAL(duration) %d usec of slice %d\n", sdr_compatible_str, tmp&0xFFFFF, tmp>>20);
    if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_TOTAL, tmp))
      goto nla_put_failure;
    return cfg80211_testmode_reply(skb);

  case OPENWIFI_CMD_SET_SLICE_START:
    if (!tb[OPENWIFI_ATTR_SLICE_START])
      return -EINVAL;
    tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_START]);
    if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
      printk("%s set SLICE_START(duration) WARNING: current slice idx %d is invalid!\n", sdr_compatible_str, priv->slice_idx);
      return -EOPNOTSUPP;
    } else {
      printk("%s set SLICE_START(duration) %d usec to slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
      xpu_api->XPU_REG_SLICE_COUNT_START_write((priv->slice_idx<<20)|tmp);
    }
    return 0;
  case OPENWIFI_CMD_GET_SLICE_START:
    skb = (struct sk_buff *)cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
    if (!skb)
      return -ENOMEM;
    tmp = (xpu_api->XPU_REG_SLICE_COUNT_START_read());
    printk("%s get SLICE_START(duration) %d usec of slice %d\n", sdr_compatible_str, tmp&0xFFFFF, tmp>>20);
    if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_START, tmp))
      goto nla_put_failure;
    return cfg80211_testmode_reply(skb);

  case OPENWIFI_CMD_SET_SLICE_END:
    if (!tb[OPENWIFI_ATTR_SLICE_END])
      return -EINVAL;
    tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_END]);
    if (priv->slice_idx>=MAX_NUM_HW_QUEUE) {
      printk("%s set SLICE_END(duration) WARNING: current slice idx %d is invalid!\n", sdr_compatible_str, priv->slice_idx);
      return -EOPNOTSUPP;
    } else {
      printk("%s set SLICE_END(duration) %d usec to slice %d\n", sdr_compatible_str, tmp, priv->slice_idx);
      xpu_api->XPU_REG_SLICE_COUNT_END_write((priv->slice_idx<<20)|tmp);
    }
    return 0;
  case OPENWIFI_CMD_GET_SLICE_END:
    skb = (struct sk_buff *)cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
    if (!skb)
      return -ENOMEM;
    tmp = (xpu_api->XPU_REG_SLICE_COUNT_END_read());
    printk("%s get SLICE_END(duration) %d usec of slice %d\n", sdr_compatible_str, tmp&0xFFFFF, tmp>>20);
    if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_END, tmp))
      goto nla_put_failure;
    return cfg80211_testmode_reply(skb);

  // case OPENWIFI_CMD_SET_SLICE_TOTAL1:
  //   if (!tb[OPENWIFI_ATTR_SLICE_TOTAL1])
  //     return -EINVAL;
  //   tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_TOTAL1]);
  //   printk("%s set SLICE_TOTAL1(duration) to %d usec\n", sdr_compatible_str, tmp);
  //   // xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_write(tmp);
  //   return 0;
  // case OPENWIFI_CMD_GET_SLICE_TOTAL1:
  //   skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
  //   if (!skb)
  //     return -ENOMEM;
  //   // tmp = (xpu_api->XPU_REG_SLICE_COUNT_TOTAL1_read());
  //   if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_TOTAL1, tmp))
  //     goto nla_put_failure;
  //   return cfg80211_testmode_reply(skb);

  // case OPENWIFI_CMD_SET_SLICE_START1:
  //   if (!tb[OPENWIFI_ATTR_SLICE_START1])
  //     return -EINVAL;
  //   tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_START1]);
  //   printk("%s set SLICE_START1(duration) to %d usec\n", sdr_compatible_str, tmp);
  //   // xpu_api->XPU_REG_SLICE_COUNT_START1_write(tmp);
  //   return 0;
  // case OPENWIFI_CMD_GET_SLICE_START1:
  //   skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
  //   if (!skb)
  //     return -ENOMEM;
  //   // tmp = (xpu_api->XPU_REG_SLICE_COUNT_START1_read());
  //   if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_START1, tmp))
  //     goto nla_put_failure;
  //   return cfg80211_testmode_reply(skb);

  // case OPENWIFI_CMD_SET_SLICE_END1:
  //   if (!tb[OPENWIFI_ATTR_SLICE_END1])
  //     return -EINVAL;
  //   tmp = nla_get_u32(tb[OPENWIFI_ATTR_SLICE_END1]);
  //   printk("%s set SLICE_END1(duration) to %d usec\n", sdr_compatible_str, tmp);
  //   // xpu_api->XPU_REG_SLICE_COUNT_END1_write(tmp);
  //   return 0;
  // case OPENWIFI_CMD_GET_SLICE_END1:
  //   skb = cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
  //   if (!skb)
  //     return -ENOMEM;
  //   // tmp = (xpu_api->XPU_REG_SLICE_COUNT_END1_read());
  //   if (nla_put_u32(skb, OPENWIFI_ATTR_SLICE_END1, tmp))
  //     goto nla_put_failure;
  //   return cfg80211_testmode_reply(skb);

  case OPENWIFI_CMD_SET_RSSI_TH:
    if (!tb[OPENWIFI_ATTR_RSSI_TH])
      return -EINVAL;
    tmp = nla_get_u32(tb[OPENWIFI_ATTR_RSSI_TH]);
    // printk("%s set RSSI_TH to %d\n", sdr_compatible_str, tmp);
    // xpu_api->XPU_REG_LBT_TH_write(tmp);
    // return 0;
    printk("%s WARNING Please use command: sdrctl dev sdr0 set reg drv_xpu 0 reg_value! (1~2047, 0 means AUTO)!\n", sdr_compatible_str);
    return -EOPNOTSUPP;
  case OPENWIFI_CMD_GET_RSSI_TH:
    skb = (struct sk_buff *)cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
    if (!skb)
      return -ENOMEM;
    tmp_int = rssi_half_db_to_rssi_dbm(xpu_api->XPU_REG_LBT_TH_read(), priv->rssi_correction); //rssi_dbm
    tmp = (-tmp_int);
    if (nla_put_u32(skb, OPENWIFI_ATTR_RSSI_TH, tmp))
      goto nla_put_failure;
    return cfg80211_testmode_reply(skb);

  case OPENWIFI_CMD_SET_TSF:
    printk("openwifi_set_tsf_1");
    if ( (!tb[OPENWIFI_ATTR_HIGH_TSF]) || (!tb[OPENWIFI_ATTR_LOW_TSF]) )
        return -EINVAL;
    printk("openwifi_set_tsf_2");
    tsft_high = nla_get_u32(tb[OPENWIFI_ATTR_HIGH_TSF]);
    tsft_low  = nla_get_u32(tb[OPENWIFI_ATTR_LOW_TSF]);
    xpu_api->XPU_REG_TSF_LOAD_VAL_write(tsft_high,tsft_low);
    printk("%s openwifi_set_tsf: %08x%08x\n", sdr_compatible_str,tsft_high,tsft_low);
    return 0;

  case REG_CMD_SET:
    if ( (!tb[REG_ATTR_ADDR]) || (!tb[REG_ATTR_VAL]) )
      return -EINVAL;
    reg_addr = nla_get_u32(tb[REG_ATTR_ADDR]);
    reg_val  = nla_get_u32(tb[REG_ATTR_VAL]);
    reg_cat = ((reg_addr>>16)&0xFFFF);
    reg_addr = (reg_addr&0xFFFF);
    reg_addr_idx = (reg_addr>>2);
    printk("%s recv set cmd reg cat %d addr %08x val %08x idx %d\n", sdr_compatible_str, reg_cat, reg_addr, reg_val, reg_addr_idx);
    if (reg_cat==SDRCTL_REG_CAT_RF) {
      // printk("%s WARNING reg cat 1 (rf) is not supported yet!\n", sdr_compatible_str);
      // return -EOPNOTSUPP;
      if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_RF_REG) {
        priv->rf_reg_val[reg_addr_idx]=reg_val;
        if (reg_addr_idx==RF_TX_REG_IDX_ATT) {//change the tx ON att (if a RF chain is ON)
          tmp = ad9361_get_tx_atten(priv->ad9361_phy, 1);
          printk("%s ad9361_get_tx_atten ant0 %d\n",sdr_compatible_str, tmp);
          if (tmp<AD9361_RADIO_OFF_TX_ATT) {
            err = ad9361_set_tx_atten(priv->ad9361_phy, AD9361_RADIO_ON_TX_ATT+reg_val, true, false, true);
            if (err < 0) {
              printk("%s WARNING ad9361_set_tx_atten ant0 %d FAIL!\n",sdr_compatible_str, AD9361_RADIO_ON_TX_ATT+reg_val);
              return -EIO;
            } else {
              printk("%s ad9361_set_tx_atten ant0 %d OK\n",sdr_compatible_str, AD9361_RADIO_ON_TX_ATT+reg_val);
            }
          }
          tmp = ad9361_get_tx_atten(priv->ad9361_phy, 2);
          printk("%s ad9361_get_tx_atten ant1 %d\n",sdr_compatible_str, tmp);
          if (tmp<AD9361_RADIO_OFF_TX_ATT) {
            err = ad9361_set_tx_atten(priv->ad9361_phy, AD9361_RADIO_ON_TX_ATT+reg_val, false, true, true);
            if (err < 0) {
              printk("%s WARNING ad9361_set_tx_atten ant1 %d FAIL!\n",sdr_compatible_str, AD9361_RADIO_ON_TX_ATT+reg_val);
              return -EIO;
            } else {
              printk("%s ad9361_set_tx_atten ant1 %d OK\n",sdr_compatible_str, AD9361_RADIO_ON_TX_ATT+reg_val);
            }
          }
        } else if (reg_addr_idx==RF_TX_REG_IDX_FREQ_MHZ || reg_addr_idx==RF_RX_REG_IDX_FREQ_MHZ) { // apply the tx and rx fo
          channel_conf_tmp.chandef.chan->center_freq = reg_val;
          ad9361_rf_set_channel(hw, &channel_conf_tmp);
          priv->stat.restrict_freq_mhz = reg_val;
          // clk_set_rate(priv->ad9361_phy->clks[TX_RFPLL], ( ((u64)1000000ull)*((u64)priv->rf_reg_val[RF_TX_REG_IDX_FREQ_MHZ]) )>>1 );
          // ad9361_tx_calibration(priv, priv->rf_reg_val[RF_TX_REG_IDX_FREQ_MHZ]);
          // printk("%s clk_set_rate TX_RFPLL %dMHz done\n",sdr_compatible_str, priv->rf_reg_val[RF_TX_REG_IDX_FREQ_MHZ]);
        } 
        // else if (reg_addr_idx==RF_RX_REG_IDX_FREQ_MHZ) { // apply the rx fo
        //   channel_conf_tmp.chandef.chan->center_freq = reg_val;
        //   ad9361_rf_set_channel(hw, &channel_conf_tmp);
        //   // clk_set_rate(priv->ad9361_phy->clks[RX_RFPLL], ( ((u64)1000000ull)*((u64)priv->rf_reg_val[RF_RX_REG_IDX_FREQ_MHZ]) )>>1 );
        //   // openwifi_rf_rx_update_after_tuning(priv, priv->rf_reg_val[RF_RX_REG_IDX_FREQ_MHZ]);
        //   // printk("%s clk_set_rate RX_RFPLL %dMHz done\n",sdr_compatible_str, priv->rf_reg_val[RF_RX_REG_IDX_FREQ_MHZ]);
        // }
      } else {
        printk("%s WARNING reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
        return -EOPNOTSUPP;
      }
    }
    else if (reg_cat==SDRCTL_REG_CAT_RX_INTF)
      rx_intf_api->reg_write(reg_addr,reg_val);
    else if (reg_cat==SDRCTL_REG_CAT_TX_INTF)
      tx_intf_api->reg_write(reg_addr,reg_val);
    else if (reg_cat==SDRCTL_REG_CAT_RX)
      openofdm_rx_api->reg_write(reg_addr,reg_val);
    else if (reg_cat==SDRCTL_REG_CAT_TX)
      openofdm_tx_api->reg_write(reg_addr,reg_val);
    else if (reg_cat==SDRCTL_REG_CAT_XPU)
      xpu_api->reg_write(reg_addr,reg_val);
    else if (reg_cat==SDRCTL_REG_CAT_DRV_RX) {
      if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
        if (reg_addr_idx==DRV_RX_REG_IDX_ANT_CFG) {
          tmp = openwifi_set_antenna(hw, (priv->drv_tx_reg_val[reg_addr_idx]==0?1:2), (reg_val==0?1:2));
          if (tmp) {
            printk("%s WARNING openwifi_set_antenna return %d!\n", sdr_compatible_str, tmp);
            return -EIO;
          } else {
            priv->drv_rx_reg_val[reg_addr_idx]=reg_val;
          }
        } else {
          priv->drv_rx_reg_val[reg_addr_idx]=reg_val;
          if (reg_addr_idx==DRV_RX_REG_IDX_DEMOD_TH) {
            openofdm_rx_api->OPENOFDM_RX_REG_POWER_THRES_write((OPENOFDM_RX_DC_RUNNING_SUM_TH_INIT<<16)|rssi_dbm_to_rssi_half_db((reg_val==0?OPENOFDM_RX_RSSI_DBM_TH_DEFAULT:(-reg_val)), priv->rssi_correction));
          }
        }
      } else {
        printk("%s WARNING reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
        return -EOPNOTSUPP;
      }
    }
    else if (reg_cat==SDRCTL_REG_CAT_DRV_TX) {
      if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
        if ((reg_addr_idx == DRV_TX_REG_IDX_RATE || reg_addr_idx == DRV_TX_REG_IDX_RATE_HT) &&
            (reg_val != 0 && (!((reg_val&0xF)>=4 && (reg_val&0xF)<=11)) ) ) {
          printk("%s WARNING rate override value should be 0 or 4~11!\n", sdr_compatible_str);
          return -EOPNOTSUPP;
        } else {
          if (reg_addr_idx==DRV_TX_REG_IDX_ANT_CFG) {
            tmp = openwifi_set_antenna(hw, reg_val+1, priv->drv_rx_reg_val[reg_addr_idx]+1);
            if (tmp) {
              printk("%s WARNING openwifi_set_antenna return %d!\n", sdr_compatible_str, tmp);
              return -EIO;
            } else {
              priv->drv_tx_reg_val[reg_addr_idx]=reg_val;
            }
          } else {
            priv->drv_tx_reg_val[reg_addr_idx]=reg_val;
          }
        }
      } else {
        printk("%s WARNING reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
        return -EOPNOTSUPP;
      }
    }
    else if (reg_cat==SDRCTL_REG_CAT_DRV_XPU) {
      if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
        priv->drv_xpu_reg_val[reg_addr_idx]=reg_val;
        if (reg_addr_idx==DRV_XPU_REG_IDX_LBT_TH) {
          if (reg_val) {
            tmp_int = (-reg_val); // rssi_dbm
            tmp = rssi_dbm_to_rssi_half_db(tmp_int, priv->rssi_correction);
            xpu_api->XPU_REG_LBT_TH_write( tmp );
            printk("%s override FPGA LBT threshold to %d(%ddBm). The last_auto_fpga_lbt_th %d(%ddBm). rssi corr %d (%d/%dMHz)\n", sdr_compatible_str, tmp, tmp_int, priv->last_auto_fpga_lbt_th, rssi_half_db_to_rssi_dbm(priv->last_auto_fpga_lbt_th, priv->rssi_correction), priv->rssi_correction, priv->actual_tx_lo, priv->actual_rx_lo);
          } else {
            xpu_api->XPU_REG_LBT_TH_write(priv->last_auto_fpga_lbt_th);
            printk("%s Restore last_auto_fpga_lbt_th %d(%ddBm) to FPGA. ad9361_rf_set_channel will take control. rssi corr %d (%d/%dMHz)\n", sdr_compatible_str, priv->last_auto_fpga_lbt_th, rssi_half_db_to_rssi_dbm(priv->last_auto_fpga_lbt_th, priv->rssi_correction), priv->rssi_correction, priv->actual_tx_lo, priv->actual_rx_lo);
          }
        }
      } else {
        printk("%s WARNING reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
        return -EOPNOTSUPP;
      }
    }
    else {
      printk("%s WARNING reg cat %d is not supported yet!\n", sdr_compatible_str, reg_cat);
      return -EOPNOTSUPP;
    }
    
    return 0;
  case REG_CMD_GET:
    skb = (struct sk_buff *)cfg80211_testmode_alloc_reply_skb(hw->wiphy, nla_total_size(sizeof(u32)));
    if (!skb)
      return -ENOMEM;
    reg_addr = nla_get_u32(tb[REG_ATTR_ADDR]);
    reg_cat = ((reg_addr>>16)&0xFFFF);
    reg_addr = (reg_addr&0xFFFF);
    reg_addr_idx = (reg_addr>>2);
    printk("%s recv get cmd reg cat %d addr %08x idx %d\n", sdr_compatible_str, reg_cat, reg_addr, reg_addr_idx);
    if (reg_cat==SDRCTL_REG_CAT_RF) {
      // printk("%s WARNING reg cat 1 (rf) is not supported yet!\n", sdr_compatible_str);
      // tmp = 0xFFFFFFFF;
      // return -EOPNOTSUPP;
      if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_RF_REG) {
        tmp = priv->rf_reg_val[reg_addr_idx];
      } else {
        printk("%s WARNING reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
        return -EOPNOTSUPP;
      }
    }
    else if (reg_cat==SDRCTL_REG_CAT_RX_INTF)
      tmp = rx_intf_api->reg_read(reg_addr);
    else if (reg_cat==SDRCTL_REG_CAT_TX_INTF)
      tmp = tx_intf_api->reg_read(reg_addr);
    else if (reg_cat==SDRCTL_REG_CAT_RX)
      tmp = openofdm_rx_api->reg_read(reg_addr);
    else if (reg_cat==SDRCTL_REG_CAT_TX)
      tmp = openofdm_tx_api->reg_read(reg_addr);
    else if (reg_cat==SDRCTL_REG_CAT_XPU)
      tmp = xpu_api->reg_read(reg_addr);
    else if (reg_cat==SDRCTL_REG_CAT_DRV_RX) {
      if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
        tmp = priv->drv_rx_reg_val[reg_addr_idx];
        if (reg_addr_idx==DRV_RX_REG_IDX_ANT_CFG)
          openwifi_get_antenna(hw, &tsft_high, &tsft_low);
      } else {
        printk("%s WARNING reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
        return -EOPNOTSUPP;
      }
    }
    else if (reg_cat==SDRCTL_REG_CAT_DRV_TX) {
      if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
        tmp = priv->drv_tx_reg_val[reg_addr_idx];
        if (reg_addr_idx==DRV_TX_REG_IDX_ANT_CFG)
          openwifi_get_antenna(hw, &tsft_high, &tsft_low);
      } else {
        printk("%s WARNING reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
        return -EOPNOTSUPP;
      }
    }
    else if (reg_cat==SDRCTL_REG_CAT_DRV_XPU) {
      if (reg_addr_idx>=0 && reg_addr_idx<MAX_NUM_DRV_REG) {
        if (reg_addr_idx==DRV_XPU_REG_IDX_LBT_TH) {
          tmp = xpu_api->XPU_REG_LBT_TH_read();//rssi_half_db
          tmp_int = rssi_half_db_to_rssi_dbm(tmp, priv->rssi_correction); //rssi_dbm
          printk("%s FPGA LBT threshold %d(%ddBm). The last_auto_fpga_lbt_th %d(%ddBm). rssi corr %d (%d/%dMHz)\n", sdr_compatible_str, tmp, tmp_int, priv->last_auto_fpga_lbt_th, rssi_half_db_to_rssi_dbm(priv->last_auto_fpga_lbt_th, priv->rssi_correction), priv->rssi_correction, priv->actual_tx_lo, priv->actual_rx_lo);
        }
        tmp = priv->drv_xpu_reg_val[reg_addr_idx];
      } else {
        printk("%s WARNING reg_addr_idx %d is out of range!\n", sdr_compatible_str, reg_addr_idx);
        return -EOPNOTSUPP;
      }
    }
    else {
      printk("%s WARNING reg cat %d is not supported yet!\n", sdr_compatible_str, reg_cat);
      return -EOPNOTSUPP;
    }

    if (nla_put_u32(skb, REG_ATTR_VAL, tmp))
      goto nla_put_failure;
    return cfg80211_testmode_reply(skb);

  default:
    return -EOPNOTSUPP;
  }

 nla_put_failure:
  dev_kfree_skb(skb);
  return -ENOBUFS;
}
