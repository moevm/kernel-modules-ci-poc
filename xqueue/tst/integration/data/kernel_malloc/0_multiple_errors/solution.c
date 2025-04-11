#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "checker.h"

static void *p0;
static int *p1;

static int __init solution_init(void)
{
	size_t s0, s1;

	pr_info("[kernel_mooc]: solution init");
	s0 = get_void_size();
	s1 = get_int_array_size();

	p0 = kmalloc(s0/4, GFP_KERNEL);
	p1 = kcalloc(s1, sizeof(*p1), GFP_KERNEL);

	submit_void_ptr(p0);
	submit_int_array_ptr(p1);

	return 0;
}

static void __exit solution_exit(void)
{
	pr_info("[kernel_mooc]: solution exit");
	checker_kfree(p0);
}

module_init(solution_init);
module_exit(solution_exit);

MODULE_LICENSE("GPL");

