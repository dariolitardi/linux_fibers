#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xe15704bf, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x74ae8928, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x911c0168, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x2c49175, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0xa9613291, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x8133c1b9, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x8fcf017f, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x42deffa7, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x99c21189, __VMLINUX_SYMBOL_STR(try_module_get) },
	{ 0xce8a9368, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

