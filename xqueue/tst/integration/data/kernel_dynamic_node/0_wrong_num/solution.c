#include <linux/init.h>           
#include <linux/module.h>         
#include <linux/device.h>         
#include <linux/kernel.h>         
#include <linux/fs.h>             
#include <asm/uaccess.h>          
 
MODULE_LICENSE("GPL");            
MODULE_AUTHOR("test solution");    
MODULE_DESCRIPTION("Dumb chardev driver");
MODULE_VERSION("0.1");            
 
#define BUFFER_SIZE 250
#define CLASS_NAME "test_chardev_class"

static char *node_name = "node_name";
module_param(node_name, charp, 0000);
MODULE_PARM_DESC(node_name, "A node name");

static struct class*  chardevClass  = NULL; 
static struct device* chardevDevice = NULL; 

static int  majorNumber=-1;   
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
   majorNumber = register_chrdev(0, node_name, &fops);
   if (majorNumber < 0){
      printk(KERN_ALERT "DummyChardev: failed to register existing device\n");
      return majorNumber;
   }

   chardevClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(chardevClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, node_name);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(chardevClass);          // Correct way to return an error on a pointer
   }
 
   printk(KERN_INFO "DummyChardev: existing device registered successfuly\n");
   // Register the device driver
   chardevDevice = device_create(chardevClass, NULL, MKDEV(majorNumber, 0), NULL, node_name);
   if (IS_ERR(chardevDevice)){               // Clean up if there is an error
      class_destroy(chardevClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, node_name);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(chardevDevice);
   }

   sprintf(message, "%d\n", -9999);
   return 0;
}
 
static void __exit solution_exit(void){
   device_destroy(chardevClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(chardevClass);                          // unregister the device class
   class_destroy(chardevClass);
   unregister_chrdev(majorNumber, node_name);             // unregister the major number
   printk(KERN_INFO "DummyChardev: rmmod\n");
}
 
static int dev_open(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "DummyChardev: dev_open\n");
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
   return len;
}
 
static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "DummyChardev: dev_release\n");
   return 0;
}
 
module_init(solution_init);
module_exit(solution_exit);
