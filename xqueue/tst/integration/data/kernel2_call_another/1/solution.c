#include <linux/module.h>
#include <linux/kernel.h>
#include "checker.h"

int init_module(void)
{
	printk(KERN_INFO "Hello world\n");
	call_me("Hello from my module!");

	return 0;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Goodbye world\n");
}

MODULE_LICENSE("GPL");
