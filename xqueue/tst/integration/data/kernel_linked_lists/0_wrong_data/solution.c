#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>

static struct kobject *my_kobject;
static struct module *cursor_mod = THIS_MODULE;

static ssize_t modc;
static char **module_names;

void strings_bubble_sort(char **arr, int size) {
  int i, j;
  char *tmp;

}

static ssize_t my_sys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  int i;
  ssize_t len = 0;

  for (i = 0; i < modc; i++)
    len += sprintf(&buf[len], "%s\n", module_names[i]);

  return len;
}
static struct kobj_attribute my_sys_attr = __ATTR_RO(my_sys);

int init_module(void) {
  int err = 0;
  module_names = kmalloc(512, GFP_KERNEL);
  modc = 1;

  // Init kobjects
  my_kobject = kobject_create_and_add("my_kobject", kernel_kobj);
  err = sysfs_create_file(my_kobject, &my_sys_attr.attr);

  // Set current module as first
  printk(KERN_INFO "Linked Lists: Module loaded\n");
  module_names[0] = THIS_MODULE->name;

  // Save module names
  list_for_each_entry(cursor_mod, &(THIS_MODULE->list), list) module_names[modc++] = cursor_mod->name;
  modc--;
  printk(KERN_INFO "Linked Lists: Module count is %d\n", (int)modc);

  // Sort module names
  strings_bubble_sort(module_names, modc);
  printk(KERN_INFO "Linked Lists: Modules are sorted: %s, %s, ..., %s\n", module_names[0], module_names[1], module_names[modc - 1]);

  return err;
}

void cleanup_module(void) {
  sysfs_remove_file(my_kobject, &my_sys_attr.attr);
  kobject_del(my_kobject);

  printk(KERN_INFO "Linked Lists: Goodbye world\n");
}

MODULE_LICENSE("GPL");
