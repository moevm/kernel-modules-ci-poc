#include <linux/module.h>
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

#define MAX_MESSAGE_LEN 255
#define log(level, args...) { \
	char tmp[MAX_MESSAGE_LEN]; \
	char func_name[MAX_MESSAGE_LEN]; \
	sprintf(tmp, args); \
	sprintf(func_name, "[%s]:", __func__); \
	level("[%s]%-20s %s\n", "kernel_mooc", func_name, tmp); \
}
#define info(args...) log(pr_info, args)
#define alert(args...) log(pr_alert, args)

#define MAJOR_NUMBER 240
#define DEVICE_NAME "solution_node"

/* Messages to store in char device */
static char             msg[MAX_MESSAGE_LEN];
static char		*default_msg = "";
static char		*saved_msg = &msg[0];
static char		*message;
/* Timeout and timer to show saved message */
static int		timeout;
static struct hrtimer	timer;
/* Flag to mark reading process */
static int		finished_reading;

/* Module arguments */
module_param(default_msg, charp, 0000);
MODULE_PARM_DESC(default_msg, "Default message to show");
module_param(timeout, int, 0644);
MODULE_PARM_DESC(timeout, "Time in ms to show saved message");

static enum		hrtimer_restart timer_handler(struct hrtimer *);
static int		dev_open(struct inode *, struct file *);
static int		dev_release(struct inode *, struct file *);
static ssize_t	dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t	dev_write(struct file *, const char *, size_t, loff_t *);

static const struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release
};

static int
__init solution_init(void)
{
	int err;

	info("Args passed: default_msg=%s, timeout=%dms", default_msg, timeout);

	err = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &fops);
	if (err < 0) {
		alert("Failed to register existing device");

		return err;
	}
	info("Existing device registered successfuly");

	info("Initializing timer");
	message = default_msg;
	hrtimer_init(&timer, CLOCK_REALTIME, HRTIMER_MODE_REL);
	timer.function = &timer_handler;

	return 0;
}

static void
__exit solution_exit(void)
{
	info("Canceling timer");
	hrtimer_cancel(&timer);

	info("Unregistering chardev");
	unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);

	info("Exiting");
}

static enum hrtimer_restart timer_handler(struct hrtimer *timer)
{
	info("Timer triggered. Setting message back to default");

	message = default_msg;

	return HRTIMER_NORESTART;
}

static int
dev_open(struct inode *inodep, struct file *filep)
{
	info("Device opened");

	return 0;
}

static ssize_t
dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	int bytes_read = 0;
	char *readptr  = message;

	if (finished_reading) {
		info("Reading finished");
		finished_reading = 0;

		return 0;
	}

	info("Sending '%s' to the reader", message);

	while (*readptr) {
		put_user(*(readptr++), buffer++);
		bytes_read++;
	}

	finished_reading = 1;

	info("Sent %d bytes", bytes_read);

	return bytes_read;
}

static ssize_t
dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	ktime_t period;

	info("Saving %lu characters", len);
	copy_from_user(saved_msg, buffer, len);
	snprintf(saved_msg, len, "%s", saved_msg);
	info("Saved '%s'", saved_msg);

	info("Setting up timer");
	hrtimer_cancel(&timer);
	message = saved_msg;
	period = ms_to_ktime(50);
	hrtimer_start(&timer, period, HRTIMER_MODE_REL);

	return len;
}

static int
dev_release(struct inode *inodep, struct file *filep)
{
	info("Device closed");

	return 0;
}

module_init(solution_init);
module_exit(solution_exit);
