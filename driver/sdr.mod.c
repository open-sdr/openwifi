#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xb09dfeb4, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xc995c66c, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0xff9c5062, __VMLINUX_SYMBOL_STR(platform_driver_unregister) },
	{ 0x21b1535d, __VMLINUX_SYMBOL_STR(__platform_driver_register) },
	{ 0xe6af8684, __VMLINUX_SYMBOL_STR(ieee80211_beacon_get_tim) },
	{ 0x58d2273e, __VMLINUX_SYMBOL_STR(queue_delayed_work_on) },
	{ 0xf1969a8e, __VMLINUX_SYMBOL_STR(__usecs_to_jiffies) },
	{ 0xb3f37fc5, __VMLINUX_SYMBOL_STR(ieee80211_queue_stopped) },
	{ 0xa9c4aea, __VMLINUX_SYMBOL_STR(skb_set_owner_w) },
	{ 0xa3656bac, __VMLINUX_SYMBOL_STR(skb_realloc_headroom) },
	{ 0xdf8340c8, __VMLINUX_SYMBOL_STR(ieee80211_stop_queue) },
	{ 0x51d559d1, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0x92e9f921, __VMLINUX_SYMBOL_STR(mem_map) },
	{ 0x598542b2, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0x8a4e5c74, __VMLINUX_SYMBOL_STR(skb_push) },
	{ 0xda22fef0, __VMLINUX_SYMBOL_STR(ieee80211_ctstoself_duration) },
	{ 0xe707d823, __VMLINUX_SYMBOL_STR(__aeabi_uidiv) },
	{ 0x4166ca61, __VMLINUX_SYMBOL_STR(ieee80211_unregister_hw) },
	{ 0x20e98e40, __VMLINUX_SYMBOL_STR(wiphy_rfkill_stop_polling) },
	{ 0xbb9df43, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0x42813b5e, __VMLINUX_SYMBOL_STR(ieee80211_free_hw) },
	{ 0xb24e2552, __VMLINUX_SYMBOL_STR(ieee80211_register_hw) },
	{ 0x79aa04a2, __VMLINUX_SYMBOL_STR(get_random_bytes) },
	{ 0xf88c3301, __VMLINUX_SYMBOL_STR(sg_init_table) },
	{ 0x8c3d04d3, __VMLINUX_SYMBOL_STR(wiphy_apply_custom_regulatory) },
	{ 0x470edd8, __VMLINUX_SYMBOL_STR(cf_axi_dds_datasel) },
	{ 0xbb592165, __VMLINUX_SYMBOL_STR(platform_bus_type) },
	{ 0x71b79e15, __VMLINUX_SYMBOL_STR(ad9361_spi_read) },
	{ 0x43dee1c8, __VMLINUX_SYMBOL_STR(ad9361_spi_to_phy) },
	{ 0x57de65fe, __VMLINUX_SYMBOL_STR(bus_find_device) },
	{ 0xb0649b67, __VMLINUX_SYMBOL_STR(spi_bus_type) },
	{ 0xfb58f556, __VMLINUX_SYMBOL_STR(ieee80211_alloc_hw_nm) },
	{ 0xab41ed6, __VMLINUX_SYMBOL_STR(of_match_node) },
	{ 0xff178f6, __VMLINUX_SYMBOL_STR(__aeabi_idivmod) },
	{ 0x43792f3e, __VMLINUX_SYMBOL_STR(wiphy_rfkill_start_polling) },
	{ 0x9dcef554, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x2072ee9b, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0x3c53fb1e, __VMLINUX_SYMBOL_STR(irq_of_parse_and_map) },
	{ 0xaf26042f, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0xcbc3f1f, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xad4e5157, __VMLINUX_SYMBOL_STR(dma_alloc_from_dev_coherent) },
	{ 0x85fb4afb, __VMLINUX_SYMBOL_STR(dma_request_slave_channel) },
	{ 0xdca9ce1, __VMLINUX_SYMBOL_STR(ieee80211_rx_irqsafe) },
	{ 0x9d669763, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x78e6caa, __VMLINUX_SYMBOL_STR(skb_put) },
	{ 0x2654153e, __VMLINUX_SYMBOL_STR(__netdev_alloc_skb) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0xc12866b6, __VMLINUX_SYMBOL_STR(ieee80211_wake_queue) },
	{ 0x5fc56a46, __VMLINUX_SYMBOL_STR(_raw_spin_unlock) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0x93c5e69b, __VMLINUX_SYMBOL_STR(skb_pull) },
	{ 0x39e28977, __VMLINUX_SYMBOL_STR(ieee80211_tx_status_irqsafe) },
	{ 0x9c0bd51f, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0xc1514a3b, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0xccf1e993, __VMLINUX_SYMBOL_STR(dma_release_channel) },
	{ 0x67fcf6b9, __VMLINUX_SYMBOL_STR(ad9361_tx_mute) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x16305289, __VMLINUX_SYMBOL_STR(warn_slowpath_null) },
	{ 0x87d641, __VMLINUX_SYMBOL_STR(arm_dma_ops) },
	{ 0xc3eb7426, __VMLINUX_SYMBOL_STR(dma_release_from_dev_coherent) },
	{ 0x6b06fdce, __VMLINUX_SYMBOL_STR(delayed_work_timer_fn) },
	{ 0x5ee52022, __VMLINUX_SYMBOL_STR(init_timer_key) },
	{ 0xb2d48a2e, __VMLINUX_SYMBOL_STR(queue_work_on) },
	{ 0x2d3385d3, __VMLINUX_SYMBOL_STR(system_wq) },
	{ 0xfd792e2a, __VMLINUX_SYMBOL_STR(cancel_delayed_work_sync) },
	{ 0xc0b8fd64, __VMLINUX_SYMBOL_STR(consume_skb) },
	{ 0xeae748f2, __VMLINUX_SYMBOL_STR(cfg80211_vendor_cmd_reply) },
	{ 0x2da3db58, __VMLINUX_SYMBOL_STR(nla_put) },
	{ 0x3eaaf5ed, __VMLINUX_SYMBOL_STR(__cfg80211_alloc_reply_skb) },
	{ 0x85ded073, __VMLINUX_SYMBOL_STR(nla_parse) },
	{ 0x222e7ce2, __VMLINUX_SYMBOL_STR(sysfs_streq) },
	{ 0x1e6d26a8, __VMLINUX_SYMBOL_STR(strstr) },
	{ 0x69ee7411, __VMLINUX_SYMBOL_STR(wiphy_rfkill_set_hw_state) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=mac80211,cfg80211,ad9361_drv";

MODULE_ALIAS("of:N*T*Csdr,sdr");
MODULE_ALIAS("of:N*T*Csdr,sdrC*");
