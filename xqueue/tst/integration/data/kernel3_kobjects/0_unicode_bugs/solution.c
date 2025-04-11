#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>

static int counter;﻿

static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf)
{
		return sprintf(buf, "%d", counter++);
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count)
{
        return 0;
}

static struct kobj_attribute foo_attribute = __ATTR(my_sys, 0755, foo_show, foo_store);


static struct kobject *example_kobj;


static int __init example_init(void)
{
        example_kobj = kobject_create_and_add("my_kobject", kernel_kobj);

        sysfs_create_file(example_kobj, &foo_attribute.attr);

        return 0;
}

static void __exit example_exit(void)
{
        kobject_put(example_kobj);
}

module_init(example_init);
module_exit(example_exit);

MODULE_LICENSE("GPL");