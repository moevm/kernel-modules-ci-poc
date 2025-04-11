#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>


int init_module(void) {
  printk(KERN_INFO "Linked Lists: ");

  return 0;
}

void cleanup_module(void) {

  printk(KERN_INFO "Linked Lists: Goodbye world\n");
}

MODULE_LICENSE("GPL");
