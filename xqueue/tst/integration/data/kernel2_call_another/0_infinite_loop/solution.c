#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>

int infinite_init(void)
{
	int i = 1;

	pr_info("[kernel_mooc] [%s]", __func__);
	for (;;) {
		pr_info("[kernel_mooc] Infinite loop. Iteration %d", i++);
		msleep(1000);
	}

	return 0;
}

void infinite_exit(void)
{
	pr_info("[kernel_mooc] [%s]", __func__);
}

module_init(infinite_init);
module_exit(infinite_exit);

MODULE_LICENSE("GPL");
