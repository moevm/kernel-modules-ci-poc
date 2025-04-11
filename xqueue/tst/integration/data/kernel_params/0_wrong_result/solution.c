#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/sysfs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Kanushin");

static struct kobject *my_kobject;

static int a = 2;
static int b = 1;
static int c[5] = { 1, 2, 3, 4, 5 };
static int arr_argc = 0;

static int sum = 0;

module_param(a, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(a, "int a");
module_param(b, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(b, "int b");
module_param_array(c, int, &arr_argc, 0000);
MODULE_PARM_DESC(c, "An array of integers");

static ssize_t my_sys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sum);
}
static struct kobj_attribute my_sys_attr = __ATTR_RO(my_sys);

static int __init kernel_module_parameters_init(void)
{
	int i;
	int err = 0;

	sum = 0;

	printk(KERN_INFO "kernel_module_parameters module is loading\n");
	printk(KERN_INFO "int a: %d\n", a);
	printk(KERN_INFO "int b: %d\n", b);

	for (i = 0; i < (sizeof c / sizeof (int)); i++)
	{
		printk(KERN_INFO "c[%d] = %d\n", i, c[i]);
	}
	printk(KERN_INFO "got %d arguments for myintArray.\n", arr_argc);

	sum += b;
	for (i = 0; i <  (sizeof c / sizeof (int)); i++)
	{
		sum += c[i];
	}
	printk(KERN_INFO "sum: %d\n", sum);

	my_kobject = kobject_create_and_add("my_kobject", kernel_kobj);
	err = sysfs_create_file(my_kobject, &my_sys_attr.attr);

	return err;
}

static void __exit kernel_module_parameters_exit(void)
{
	sysfs_remove_file(my_kobject, &my_sys_attr.attr);
	kobject_del(my_kobject);
	printk(KERN_INFO "kernel_module_parameters is being removed\n");
}

module_init(kernel_module_parameters_init);
module_exit(kernel_module_parameters_exit);
