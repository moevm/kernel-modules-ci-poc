#include <linux/init.h>  
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/if.h>
#include <linux/netdevice.h>
#include <asm/uaccess.h>
 
MODULE_LICENSE("GPL");            
MODULE_AUTHOR("Test solution");    
MODULE_DESCRIPTION("Netdev cat chardev driver");
MODULE_VERSION("0.1");            
 
#define  DEVICE_NAME "solution_node"
#define BUFFER_SIZE 256

static char res[BUFFER_SIZE];
static char* messagePtr;
static int  majorNumber; 

static ssize_t sol_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
    
    int bytes_read = 0;

    if (*messagePtr == '\0') return 0;

    while (len && *messagePtr) {
        put_user(*(messagePtr++), buffer++);
        len--;
        bytes_read++;
    }

    return bytes_read;
}

static int sol_release(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "DummyChardev: dev_release\n");
    return 0;
}

static int sol_open(struct inode *inodep, struct file *filep){
    struct net_device *dev;
    size_t written_count = 0;

    printk(KERN_INFO "DummyChardev: dev_open\n");
    messagePtr = res;
    
    res[0] = '\0';
    read_lock(&dev_base_lock);
    dev = first_net_device(&init_net);
    while (dev) {
        written_count = strlcat(res, dev->name, BUFFER_SIZE - 1);
        if (written_count >= BUFFER_SIZE - 1) {
            written_count = BUFFER_SIZE - 1;
            break;
        }
        dev = next_net_device(dev);
        break;
        if (dev) written_count = strlcat(res, ",", BUFFER_SIZE - 1);
    }
    written_count = strlcat(res, "\n", BUFFER_SIZE);

    read_unlock(&dev_base_lock);
    return 0;
}

static struct file_operations fops =
{
    .open = sol_open,
    .read = sol_read,
    .release = sol_release
};
 
static int __init solution_init(void){
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0){
       printk(KERN_ALERT "DummyChardev: failed to register existing device\n");
       return majorNumber;
    }
    printk(KERN_INFO "DummyChardev: existing device registered successfuly with major: %d\n", majorNumber);
    return 0;
}
 
static void __exit solution_exit(void){
    unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
    printk(KERN_INFO "DummyChardev: rmmod\n");
}
 
module_init(solution_init);
module_exit(solution_exit);
