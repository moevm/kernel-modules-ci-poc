#include <linux/interrupt.h>
#include <linux/module.h>

#define KOBJECT_FOLDER "my_kobject"
#define KOBJECT_FILE my_sys

#define HANDLER_NAME "my handler"
#define IRQ 8

static struct kobject *my_kobject;
static int call_count;

static ssize_t
KOBJECT_FILE_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
/* Prints out number of interrupts to sys */
{
	return sprintf(buf, "%d\n", call_count);
}
static struct kobj_attribute KOBJECT_FILE_attr = __ATTR_RO(KOBJECT_FILE);

irq_handler_t
my_int_handler(int irq, void *dev_id, struct pt_regs *regs)
/* int handler */
{
	call_count++;
	return (irq_handler_t) IRQ_HANDLED;
}

static int __init
solution_init(void)
/* module init */
{
	int err;

	pr_info("kernel_mooc: kernel_interrupts module loading\n");

	// Create kobject
	my_kobject = kobject_create_and_add(KOBJECT_FOLDER, kernel_kobj);
	err = sysfs_create_file(my_kobject, &KOBJECT_FILE_attr.attr);
	if (err)
		return err;

	// Register int handler
	err = request_irq(IRQ,
		(irq_handler_t) my_int_handler,
		IRQF_SHARED,
		HANDLER_NAME,
		(void *) my_int_handler);

	return err;
}

static void __exit
solution_exit(void)
/* module exit */
{
	/* remove kobject */
	sysfs_remove_file(my_kobject, &KOBJECT_FILE_attr.attr);
	kobject_del(my_kobject);

	/* unregister int handler */
	free_irq(8, (void *)(my_int_handler));
	pr_info("kernel_mooc: kernel_interrupts module unloading\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("test solution");
MODULE_DESCRIPTION("Dummy interrupt handler");
MODULE_VERSION("0.1");

module_init(solution_init);
module_exit(solution_exit);
