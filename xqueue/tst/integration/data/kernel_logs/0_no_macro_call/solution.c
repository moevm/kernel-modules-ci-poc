#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/random.h>

#include "checker.h"

#define ARR_SIZE 10

static int arr[ARR_SIZE];

static int my_array_sum(void)
{
	int i, sum = 0;

	for (i = 0; i < ARR_SIZE; i++)
		sum += arr[i];

	return sum;
}

static void generate_array(void)
{
	int i;

	for (i = 0; i < ARR_SIZE; i++)
		arr[i] = get_random_int() % 100;
}

static int __init solution_init(void)
{
	int i, checker_sum, my_sum = 0;
	char buf[1024];

	pr_info("[kernel_mooc]: solution init");

	for (i = 0; i < ARR_SIZE; i++) {
		generate_array();
		checker_sum = array_sum(arr, ARR_SIZE);
		my_sum = my_array_sum();
		pr_info("[kernel_mooc]: checker sum: %d, my sum: %d",
				checker_sum, my_sum);

		generate_output(checker_sum, arr, ARR_SIZE, buf);

		if (checker_sum == my_sum)
			pr_info("[kernel_mooc]: %s", buf);
		else
			pr_err("[kernel_mooc]: %s", buf);
	}

	return 0;
}

static void __exit solution_exit(void)
{
	pr_info("[kernel_mooc]: solution exit");
}

module_init(solution_init);
module_exit(solution_exit);

MODULE_LICENSE("GPL");

