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

static char *node_name = "node_name";
module_param(node_name, charp, 0000);
MODULE_PARM_DESC(node_name, "A node name");

 
static int __init solution_init(void){
   printk(KERN_INFO "DummyChardev: module registered successfuly\n");
   return 0;
}
 
static void __exit solution_exit(void){
   printk(KERN_INFO "DummyChardev: rmmod\n");
}
 
 
module_init(solution_init);
module_exit(solution_exit);
