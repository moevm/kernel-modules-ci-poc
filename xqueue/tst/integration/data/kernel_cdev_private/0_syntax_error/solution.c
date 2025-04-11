#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("test solution");
MODULE_DESCRIPTION("Dumb chardev driver");
MODULE_VERSION("0.1");

#define MAJOR_NUMBER 240
#define DEVICE_NAME "solution_node"
#define BUFFER_SIZE 255
#define MAX_USERS 255


syntax error
sybtax_error();

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct private_data {
  int user_id;
  char data[BUFFER_SIZE];
  int data_size;
} * current_pdata;

static int last_user_id = -1;
static struct private_data users[MAX_USERS];

static struct file_operations fops = {
    .open = dev_open, .read = dev_read, .write = dev_write, .release = dev_release,
};

static int __init solution_init(void) {
  int result = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &fops);
  if (result < 0) {
    printk(KERN_ALERT "DummyChardevPriv: init(), failed to register existing device\n");
    return result;
  }
  printk(KERN_INFO "DummyChardevPriv: init(), existing device registered successfuly\n");

  return 0;
}

static void __exit solution_exit(void) {
  unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
  printk(KERN_INFO "DummyChardevPriv: exit()\n");
}

static int dev_open(struct inode *inodep, struct file *filep) {
  int user_id = ++last_user_id;
  struct private_data *pdata = &users[user_id];

  pdata->user_id = user_id; // set user_id
  sprintf(pdata->data, "%d", user_id);
  pdata->data_size = strlen(pdata->data);
  /*
  for (i = 0; i < BUFFER_SIZE; i++) {
    pdata->data[i] = 0; // set user data
  }
  pdata->data_size = 0;
  */
  current_pdata = pdata;

  printk(KERN_INFO "DummyChardevPriv: dev_open(), user_id: %d, his data: %s, "
                   "his filep: %p\n",
         pdata->user_id, pdata->data, filep);

  return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
  int err = 0, i, data_size;

  if (current_pdata->data[0] == 0) // If no data then terminate
    return 0;

  data_size = current_pdata->data_size;
  printk(KERN_INFO "DummyChardevPriv: dev_read(), Sending %d bytes \'%s\' to user %d\n", current_pdata->data_size, current_pdata->data, current_pdata->user_id);
  err = copy_to_user(buffer, current_pdata->data, current_pdata->data_size);

  for (i = 0; i < BUFFER_SIZE; i++) {
    current_pdata->data[i] = 0; // if success just clear user's data
  }
  current_pdata->data_size = 0;

  return data_size;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
  printk(KERN_INFO "DummyChardevPriv: dev_write(), Saving %lu bytes: %s to private data of user %d\n", len, buffer, current_pdata->user_id);
  sprintf(current_pdata->data, "%s", buffer);
  current_pdata->data[len] = '\0';
  current_pdata->data_size = len + 1;
  printk(KERN_INFO "DummyChardevPriv: dev_write(), Data is: %s\n", current_pdata->data);

  return len;
}

static int dev_release(struct inode *inodep, struct file *filep) {
  printk(KERN_INFO "DummyChardevPriv: dev_release(), user_id:%d\n", current_pdata->user_id);
  return 0;
}

module_init(solution_init);
module_exit(solution_exit);
