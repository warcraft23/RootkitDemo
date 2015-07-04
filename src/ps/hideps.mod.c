#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0x15b2dc7b, "module_layout" },
	{ 0xf557ca5c, "kobject_del" },
	{ 0x8235805b, "memmove" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x1e6d26a8, "strstr" },
	{ 0x289ae517, "current_task" },
	{ 0x50eedeb8, "printk" },
	{ 0xd0d8621b, "strlen" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "D9A8C6EED7C2F729902F3FA");
