#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("test solution");
MODULE_DESCRIPTION("Dumb chardev driver");
MODULE_VERSION("0.1");

#define MAJOR_NUMBER 240
#define DEVICE_NAME "solution_node"
#define BUFFER_SIZE 255
#define MAX_USERS 255

#define seek(off, whence) dev_llseek(filep, off, whence)

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static loff_t dev_llseek(struct file *, loff_t, int);

struct private_data {
	int user_id;
	char data[BUFFER_SIZE];
	char *pos;
	struct file *filep;
};

static int last_user_id = -1;
static struct private_data users[MAX_USERS];

static const struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
	.llseek = dev_llseek,
};

static char
*pdata_end(struct private_data *pdata)
/* Returns the end of private data */
{
	char *tmp = &pdata->data[0];

	while (*tmp != '\0')
		tmp++;

	return tmp;
}

static void
init_user_pdata(struct private_data *pdata, int user_id, struct file *filep)
{
	int i;

	for (i = 0; i < BUFFER_SIZE; i++)
		pdata->data[i] = '\0';

	pdata->user_id = user_id;
	pdata->filep = filep;
	sprintf(pdata->data, "hello");
	pdata->pos = pdata_end(pdata);
}

static void
write_pdata(struct private_data *pdata, const char *buffer, size_t len)
/* Write buffer to pdata */
{
	memcpy(pdata->pos, buffer, len);
	pdata->pos = pdata_end(pdata);
}

static struct
private_data *find_pdata_by_filep(struct file *filep)
{
	int i;

	for (i = 0; i < last_user_id + 1; i++)
		if (users[i].filep == filep)
			return &users[i];

	return NULL;
}

static int __init solution_init(void)
{
	int result = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &fops);

	if (result < 0) {
		pr_alert("kernel_mooc: init(), failed to register existing device\n");
		return result;
	}
	pr_info("kernel_mooc: init(), existing device registered successfuly\n");

	return 0;
}

static void __exit solution_exit(void)
{
	unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
	pr_info("kernel_mooc: exit()\n");
}

static int
dev_open(struct inode *inodep, struct file *filep)
{
	int user_id = ++last_user_id;
	struct private_data *pdata = &users[user_id];

	init_user_pdata(pdata, user_id, filep);
	seek(0, 0);

	pr_info("kernel_mooc: dev_open(), user_id: %d, his data: %s, his filep: %p pos:%s\n",
			 pdata->user_id, pdata->data, filep, pdata->pos);

	return 0;
}

static ssize_t
dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	ssize_t bytes_copied;
	struct private_data *pdata = find_pdata_by_filep(filep);

	pr_info("kernel_mooc: dev_read(), data: %s pos: %s len: %lu offset: %llu user_id: %d\n",
			pdata->data, pdata->pos, len, *offset, pdata->user_id);

	if (pdata->pos[0] == 0) {
		pr_info("kernel_mooc: dev_read(), no data! data: %s pos: %s user_id: %d\n",
				pdata->data, pdata->pos, pdata->user_id);

		return 0;
	}

	bytes_copied = sprintf(buffer, "%s", pdata->pos);
	pdata->pos += bytes_copied;

	pr_info("kernel_mooc: dev_read(), copied: \'%s\' (%lu bytes) user_id: %d\n",
			buffer, bytes_copied, pdata->user_id);

	return bytes_copied;
}

static ssize_t
dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	struct private_data *pdata = find_pdata_by_filep(filep);

	pr_info("kernel_mppc: dev_write(), saving %lu bytes data: %.*s pos: %s offset: %llu user: %d\n",
			len,
			(int) len, buffer, pdata->pos, *offset, pdata->user_id);
	write_pdata(pdata, buffer, len);
	pr_info("kernel_mooc: dev_write(), saved data: %s pos: %s user: %d\n",
			pdata->data, pdata->pos, pdata->user_id);

	return len;
}

static int
dev_release(struct inode *inodep, struct file *filep)
{
	struct private_data *current_pdata = find_pdata_by_filep(filep);

	pr_info("kernel_mooc: dev_release(), user_id:%d\n",
			current_pdata->user_id);

	return 0;
}

static loff_t
dev_llseek(struct file *filep, loff_t off, int whence)
{
	struct private_data *pdata = find_pdata_by_filep(filep);

	pr_info("kernel_mooc: dev_seek(%lld, %d), data: %s pos: %s user_id: %d",
			off, whence, pdata->data, pdata->pos, pdata->user_id);
	switch (whence) {
	case 0: /* seek set */
		pdata->pos = pdata->data + off;
		break;
	case 1: /* seek current */
		pdata->pos = pdata->pos + off;
		break;
	case 2: /* seek end */
		pdata->pos = pdata_end(pdata) + off;
		break;
	default:
		return -EINVAL;
	}

	pr_info("kernel_mooc: dev_seek(%lld, %d) result data: %s pos: %s user_id: %d",
			off, whence, pdata->data, pdata->pos, pdata->user_id);

	// Return old position since we don't want to touch filep
	return filep->f_pos;
}

module_init(solution_init);
module_exit(solution_exit);
