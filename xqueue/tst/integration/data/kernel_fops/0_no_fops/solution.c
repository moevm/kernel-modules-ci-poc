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
 

 
static int __init solution_init(void){
   return 0;
}
 
static void __exit solution_exit(void){
}
 
 
module_init(solution_init);
module_exit(solution_exit);
