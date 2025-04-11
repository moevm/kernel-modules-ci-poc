#include <linux/module.h>
#include <linux/hrtimer.h>
#include "checker.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("test solution");
MODULE_DESCRIPTION("Simple timers");
MODULE_VERSION("0.1");

#define DELAYS_SIZE 1024

#define ms_to_ns(ms) (ms * 1000000)

static unsigned long delays[DELAYS_SIZE] = { 0 };
static int delays_size;
module_param_array(delays, ulong, &delays_size, 0000);
MODULE_PARM_DESC(delays, "Delays in ms");

static struct hrtimer	timer;
static int		period_i;
static ktime_t		period;
#define p_ms() (delays[period_i] + 50)
#define p_ns() ms_to_ns(p_ms())
#define p_kt() ktime_set(0, p_ns())
#define next_timer() {period_i++; period = p_kt(); }

static enum hrtimer_restart my_timer_handler(struct hrtimer *timer)
{
	pr_info(
		"kernel_mooc: timer #%d: %lu ms",
		period_i,
		p_ms()
	);
	next_timer();

	check_timer();

	if (p_ms() <= 0) {
		pr_info("kernel_mooc: no timers left\n");

		return HRTIMER_NORESTART;
	}

	hrtimer_forward_now(timer, period);

	return HRTIMER_RESTART;
}

static int __init kernel_timers_init(void)
{
	int err = 0;

	pr_info("kernel_mooc: kernel_timers module is loading\n");
	pr_info("kernel_mooc: got %d delays for timer\n", delays_size);

	hrtimer_init(&timer, CLOCK_REALTIME, HRTIMER_MODE_REL);
	timer.function = &my_timer_handler;
	period = p_kt();
	hrtimer_start(&timer, period, HRTIMER_MODE_REL);

	check_timer();

	return err;
}

static void __exit kernel_timers_exit(void)
{
	pr_info("kernel_mooc: kernel_timers is unloading\n");
	hrtimer_cancel(&timer);
}

module_init(kernel_timers_init);
module_exit(kernel_timers_exit);
