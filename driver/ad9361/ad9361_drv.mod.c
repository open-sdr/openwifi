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
	{ 0xb077e70a, __VMLINUX_SYMBOL_STR(clk_unprepare) },
	{ 0x2d3385d3, __VMLINUX_SYMBOL_STR(system_wq) },
	{ 0x12da5bb2, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0xff178f6, __VMLINUX_SYMBOL_STR(__aeabi_idivmod) },
	{ 0x815588a6, __VMLINUX_SYMBOL_STR(clk_enable) },
	{ 0x528c709d, __VMLINUX_SYMBOL_STR(simple_read_from_buffer) },
	{ 0x5de9ea8f, __VMLINUX_SYMBOL_STR(sysfs_remove_bin_file) },
	{ 0x39150d0b, __VMLINUX_SYMBOL_STR(iio_read_const_attr) },
	{ 0xb6e6d99d, __VMLINUX_SYMBOL_STR(clk_disable) },
	{ 0xf7802486, __VMLINUX_SYMBOL_STR(__aeabi_uidivmod) },
	{ 0x4c2ae700, __VMLINUX_SYMBOL_STR(strnstr) },
	{ 0x61080add, __VMLINUX_SYMBOL_STR(__spi_register_driver) },
	{ 0x514cc273, __VMLINUX_SYMBOL_STR(arm_copy_from_user) },
	{ 0xe4ca3b4f, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x85df9b6c, __VMLINUX_SYMBOL_STR(strsep) },
	{ 0x63150e06, __VMLINUX_SYMBOL_STR(clk_get_parent) },
	{ 0x75ce6980, __VMLINUX_SYMBOL_STR(iio_device_register) },
	{ 0x2788b4, __VMLINUX_SYMBOL_STR(debugfs_create_file) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x556e4390, __VMLINUX_SYMBOL_STR(clk_get_rate) },
	{ 0xd8ac3b5e, __VMLINUX_SYMBOL_STR(iio_device_unregister) },
	{ 0x6faf31bf, __VMLINUX_SYMBOL_STR(complete_all) },
	{ 0x93de854a, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0xe707d823, __VMLINUX_SYMBOL_STR(__aeabi_uidiv) },
	{ 0xa8344bad, __VMLINUX_SYMBOL_STR(wait_for_completion) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0x3c80c06c, __VMLINUX_SYMBOL_STR(kstrtoull) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0xd7df54ec, __VMLINUX_SYMBOL_STR(devm_iio_device_alloc) },
	{ 0x715b759f, __VMLINUX_SYMBOL_STR(of_clk_src_onecell_get) },
	{ 0x7ad8686b, __VMLINUX_SYMBOL_STR(devm_clk_register) },
	{ 0xef7f0a81, __VMLINUX_SYMBOL_STR(iio_enum_write) },
	{ 0xe0875eb1, __VMLINUX_SYMBOL_STR(kstrtobool) },
	{ 0x9dcef554, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0xd627480b, __VMLINUX_SYMBOL_STR(strncat) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x4ea56f9, __VMLINUX_SYMBOL_STR(_kstrtol) },
	{ 0x5f3d956f, __VMLINUX_SYMBOL_STR(of_find_property) },
	{ 0xdddac782, __VMLINUX_SYMBOL_STR(of_property_read_string) },
	{ 0x8c330bf7, __VMLINUX_SYMBOL_STR(driver_unregister) },
	{ 0x84b183ae, __VMLINUX_SYMBOL_STR(strncmp) },
	{ 0x73e20c1c, __VMLINUX_SYMBOL_STR(strlcpy) },
	{ 0x250113b4, __VMLINUX_SYMBOL_STR(memory_read_from_buffer) },
	{ 0x16305289, __VMLINUX_SYMBOL_STR(warn_slowpath_null) },
	{ 0xfd16e532, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x1e6d26a8, __VMLINUX_SYMBOL_STR(strstr) },
	{ 0x6e65df87, __VMLINUX_SYMBOL_STR(of_clk_add_provider) },
	{ 0x8e865d3c, __VMLINUX_SYMBOL_STR(arm_delay_ops) },
	{ 0x80366190, __VMLINUX_SYMBOL_STR(simple_open) },
	{ 0x72d8ca50, __VMLINUX_SYMBOL_STR(devm_kfree) },
	{ 0x2196324, __VMLINUX_SYMBOL_STR(__aeabi_idiv) },
	{ 0x59e5070d, __VMLINUX_SYMBOL_STR(__do_div64) },
	{ 0x43f81957, __VMLINUX_SYMBOL_STR(clk_round_rate) },
	{ 0xbb9df43, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0x41a05e0f, __VMLINUX_SYMBOL_STR(of_property_read_u64) },
	{ 0x12a38747, __VMLINUX_SYMBOL_STR(usleep_range) },
	{ 0x6261cf22, __VMLINUX_SYMBOL_STR(axiadc_set_pnsel) },
	{ 0x7bd21d1b, __VMLINUX_SYMBOL_STR(iio_enum_available_read) },
	{ 0x2396c7f0, __VMLINUX_SYMBOL_STR(clk_set_parent) },
	{ 0x7c9a7371, __VMLINUX_SYMBOL_STR(clk_prepare) },
	{ 0x5e24bb31, __VMLINUX_SYMBOL_STR(__clk_mux_determine_rate) },
	{ 0xb678366f, __VMLINUX_SYMBOL_STR(int_sqrt) },
	{ 0x895f34e, __VMLINUX_SYMBOL_STR(devm_clk_get) },
	{ 0x76d9b876, __VMLINUX_SYMBOL_STR(clk_set_rate) },
	{ 0x9650c28d, __VMLINUX_SYMBOL_STR(devm_gpiod_get_optional) },
	{ 0x822137e2, __VMLINUX_SYMBOL_STR(arm_heavy_mb) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x719e17ff, __VMLINUX_SYMBOL_STR(clk_notifier_unregister) },
	{ 0x44026f74, __VMLINUX_SYMBOL_STR(spi_write_then_read) },
	{ 0x9d669763, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xdca23593, __VMLINUX_SYMBOL_STR(sysfs_create_bin_file) },
	{ 0x21a563da, __VMLINUX_SYMBOL_STR(clk_get_accuracy) },
	{ 0x6ec7d564, __VMLINUX_SYMBOL_STR(spi_get_device_id) },
	{ 0x60091316, __VMLINUX_SYMBOL_STR(clk_notifier_register) },
	{ 0x8b40142c, __VMLINUX_SYMBOL_STR(request_firmware) },
	{ 0x7577f6ce, __VMLINUX_SYMBOL_STR(gpiod_set_value_cansleep) },
	{ 0x946d8844, __VMLINUX_SYMBOL_STR(dev_warn) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0x222e7ce2, __VMLINUX_SYMBOL_STR(sysfs_streq) },
	{ 0x676bbc0f, __VMLINUX_SYMBOL_STR(_set_bit) },
	{ 0xb2d48a2e, __VMLINUX_SYMBOL_STR(queue_work_on) },
	{ 0xb81960ca, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x2745300b, __VMLINUX_SYMBOL_STR(of_property_read_variable_u32_array) },
	{ 0x49ebacbd, __VMLINUX_SYMBOL_STR(_clear_bit) },
	{ 0x936f3bf5, __VMLINUX_SYMBOL_STR(gpiod_set_value) },
	{ 0x1e06ce17, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0xf16fb9c5, __VMLINUX_SYMBOL_STR(release_firmware) },
	{ 0xd959e354, __VMLINUX_SYMBOL_STR(iio_enum_read) },
	{ 0xc569d8ce, __VMLINUX_SYMBOL_STR(__clk_get_name) },
	{ 0x1940cc57, __VMLINUX_SYMBOL_STR(of_clk_del_provider) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("spi:ad9361");
MODULE_ALIAS("spi:ad9364");
MODULE_ALIAS("spi:ad9361-2x");
MODULE_ALIAS("spi:ad9363a");
