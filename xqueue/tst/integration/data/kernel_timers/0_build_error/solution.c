#include <linux/module.h>
#include <checker.h> 
#include <linux/ktime.h> 

extern void check_timer(void);

static long delays[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static int num = sizeof(c) / sizeof(c[0]);
module_param_array(delays, long, &num, S_IRUGO | S_IWUSR);
static struct kt_data {
	ktime_t period;
	struct hrtimers timer;
} *data;

static enum hrtimer_restart kfun(struct hrtimer *val) {
	
	int i=0;
	/*if (val == 0)
	{*/
	check_timer();
		if (delays[i + 1]!=0) 
		{
			i++;
			return hrtimer_restart;
		}
		else
		{
			return hrtimer_norestart;
		}

	/*}*/
	//возвращает рестарт или нет.
}

static int __init my_init(void)
{
	data=kmalloc(sizeof(struct kt_data), GFP_KERNEL)//выделить память
	int i = 0;
	data->period = ktimer_set(0, delays[i]);
	check_timer();
	hrtimer_init(&data->timer, CLOCK_REALTIME, HRTIMER_MOD_REL);
	data->timer.function = kfun;
	if (kfun == hrtimer_restart) 
	{ 
		i++; 
		data->period = ktimer_set(0, delays[i]);
		
		hrtimer_start (&data->timer,data->period, HRTIMER_MOD_REL)
	}
static void __exit my_exit(void) {

	//hrtimer_cancel(&data->timer);
	//
	kfree(data);
}
//module_init(my_init);
//module_exit(my_exit);

MODULE_LICENSE("GPL");
