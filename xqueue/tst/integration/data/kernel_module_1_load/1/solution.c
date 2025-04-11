#include <linux/module.h>
#include <linux/kernel.h>

int init_module(void)
{
	printk(KERN_INFO "[kernel_mooc] Hello world 1.\n");

	return 0;
}

void cleanup_module(void)
{
	printk(KERN_INFO "[kernel_mooc] Goodbye world 1.\n");
}
