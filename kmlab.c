#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h> 
#include <linux/types.h>
#include "kmlab_given.h"
// Include headers as needed ...


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dahle"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 Lab 4");

#define DEBUG 1

// Global variables as needed ...
#define PROC_DIR_NAME "kmlab" 
#define PROC_FILE_NAME "status" 
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;

// Linked list head
struct list_head my_list;

// Linked list struct
struct ll_struct{
   struct list_head list;
   int PID;
   int CPUTime;
};

static struct proc_ops proc_fops = { 
   // .proc_read = procfs_read, 
   // .proc_write = procfs_write, 
   // .proc_open = procfs_open, 
   // .proc_release = procfs_close, 
}; 

// adds a item to the linked list
int add_node(int PID, int CPUTime)
{
   struct ll_struct *new_node = kmalloc((size_t) (sizeof(struct ll_struct)), GFP_KERNEL);
   if (!new_node) {
      printk(KERN_INFO
             "Memory allocation failed, this should never fail due to GFP_KERNEL flag\n");
      return 1;
   }
   new_node->PID = PID;
   new_node->CPUTime = CPUTime;
   list_add_tail(&(new_node->list), &my_list);
   return 0;
}

// deletes a node from the linked list
int delete_node(int PID)
{
   struct ll_struct *entry = NULL, *n;
   // list_for_each_entry(entry, &my_list, list) {
   list_for_each_entry_safe(entry, n, &my_list, list) {
      if (entry->PID == PID) {
         printk(KERN_INFO "Found the element %d\n",
                entry->PID);
         list_del(&(entry->list));
         kfree(entry);
         return 0;
      }
   }
   printk(KERN_INFO "Could not find the element %d\n", PID);
   return 1;
}

// kmlab_init - Called when module is loaded
int __init kmlab_init(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE LOADING\n");
   #endif
   // Insert your code here ...

   // create the proc directory and file
   proc_dir = proc_mkdir(PROC_DIR_NAME, NULL);
   proc_file = proc_create(PROC_FILE_NAME, 0666, proc_dir, &proc_fops);
   if (NULL == proc_file) { 
      pr_alert("Error:Could not initialize /proc/%s\n", PROC_FILE_NAME); 
      return -ENOMEM; 
    } 
   
   pr_info("KMLAB MODULE LOADED\n");
   return 0;   
}

// kmlab_exit - Called when module is unloaded
void __exit kmlab_exit(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE UNLOADING\n");
   #endif
   // Insert your code here ...
   proc_remove(proc_dir);
   

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(kmlab_init);
module_exit(kmlab_exit);
