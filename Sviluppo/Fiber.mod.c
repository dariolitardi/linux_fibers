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
	{ 0x8fcf017f, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x42deffa7, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x638fe045, __VMLINUX_SYMBOL_STR(unregister_kprobe) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x512b1d19, __VMLINUX_SYMBOL_STR(register_kprobe) },
	{ 0x8133c1b9, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x2c49175, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0xa9613291, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x74ae8928, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x911c0168, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0x9e3c7a4c, __VMLINUX_SYMBOL_STR(inc_nlink) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0xce0f5142, __VMLINUX_SYMBOL_STR(drop_nlink) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xb5419b40, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xc671e369, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x2ea2c95c, __VMLINUX_SYMBOL_STR(__x86_indirect_thunk_rax) },
	{ 0x1430a3de, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xafd29968, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x212caf42, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "D7784E241B7224342B9391A");
