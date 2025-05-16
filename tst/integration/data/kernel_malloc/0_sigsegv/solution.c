#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "checker.h"

static void *p0;
static int *p1;
static struct device *p2;

static int __init solution_init(void)
{
	size_t s0, s1, s2;
	int arr[255] = {0};

	pr_info("[kernel_mooc]: solution init");
	s0 = get_void_size();
	s1 = get_int_array_size();
	s2 = sizeof(*p2);

	p0 = kmalloc(s0, GFP_KERNEL);
	p1 = arr;
	p2 = kmalloc(s2, GFP_KERNEL);

	submit_void_ptr(p0);
	submit_int_array_ptr(p1);
	submit_struct_ptr(p2);

	return 0;
}

static void __exit solution_exit(void)
{
	pr_info("[kernel_mooc]: solution exit");
	checker_kfree(p0);
	checker_kfree(p1);
	checker_kfree(p2);
}

module_init(solution_init);
module_exit(solution_exit);

MODULE_LICENSE("GPL");
