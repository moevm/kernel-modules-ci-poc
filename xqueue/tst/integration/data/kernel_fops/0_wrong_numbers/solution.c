#include <linux/init.h>           
#include <linux/module.h>         
#include <linux/device.h>         
#include <linux/kernel.h>         
#include <linux/fs.h>             
#include <asm/uaccess.h>          
#define  DEVICE_NAME "solution_node"   
 
MODULE_LICENSE("GPL");            
MODULE_AUTHOR("test solution");    
MODULE_DESCRIPTION("Dumb chardev driver");
MODULE_VERSION("0.1");            
 
#define BUFFER_SIZE 250

static size_t  openCount=0;
static size_t  bytes_written=0;
static int  majorNumber=240;   
static char message[BUFFER_SIZE];  
static char* messagePtr; 

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
 
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};
 
static int __init solution_init(void){
   int result = register_chrdev(majorNumber, DEVICE_NAME, &fops);
   if (result < 0){
      printk(KERN_ALERT "DummyChardev: failed to register existing device\n");
      return result;
   }
   printk(KERN_INFO "DummyChardev: existing device registered successfuly\n");
   return 0;
}
 
static void __exit solution_exit(void){
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "DummyChardev: rmmod\n");
}
 
static int dev_open(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "DummyChardev: dev_open\n");
   openCount++;
   sprintf(message, "100 111\n");
   messagePtr = message;
   
   return 0;
}
 
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
  int bytes_read = 0;

  if (*messagePtr == 0)
    return 0;

  while (len && *messagePtr) {
    put_user(*(messagePtr++), buffer++);
    len--;
    bytes_read++;
  }

  return bytes_read;
}
 
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   printk(KERN_INFO "DummyChardev: Received %zu characters from the user\n", len);
   bytes_written += len;
   return len;
}
 
static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "DummyChardev: dev_release\n");
   return 0;
}
 
module_init(solution_init);
module_exit(solution_exit);
