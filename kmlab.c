#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h> 
#include <linux/types.h>
#include "kmlab_given.h"
// Include headers as needed ...
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

/* -------------------------------- Metadata -------------------------------- */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dahle"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 Lab 4");

/* -------------------------------------------------------------------------- */
/*                              Global Variables                              */
/* -------------------------------------------------------------------------- */
#define DEBUG 1
/* -------------------------------- procfile -------------------------------- */
#define PROC_DIR_NAME "kmlab" 
#define PROC_FILE_NAME "status" 
#define PROCFS_MAX_SIZE 1024
static unsigned long procfs_buffer_size = 0; 
static char procfs_buffer[PROCFS_MAX_SIZE] = "";
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;

/* ----------------------------- timer variables ---------------------------- */
static struct timer_list my_timer;
int time_interval = 5000;

// define the spin lock
// static DEFINE_SPINLOCK(my_lock);

/* ------------------------------- linked list ------------------------------ */
// Linked list head
struct list_head my_list;

// Linked list struct
struct ll_struct{
   struct list_head list;
   int PID;
   int CPUTime;
};

/* -------------------------------- Spin Lock ------------------------------- */
static DEFINE_SPINLOCK(my_lock);
//TODO: need to lock all shared datastructures and variables

/* ------------------------------- Work Queue ------------------------------- */
static struct work_struct my_work;


/* -------------------------------------------------------------------------- */
/*                             Linked List Helpers                            */
/* -------------------------------------------------------------------------- */

// adds a item to the linked list
int add_node(int PID)
{
   struct ll_struct *new_node = kmalloc((size_t) (sizeof(struct ll_struct)), GFP_KERNEL);
   if (!new_node) {
      printk(KERN_INFO
             "Memory allocation failed, this should never fail due to GFP_KERNEL flag\n");
      return 1;
   }
   new_node->PID = PID;
   new_node->CPUTime = 0;
   list_add_tail(&(new_node->list), &my_list);
   return 0;
}

void show_list(void)
{
   unsigned long flags;
   struct ll_struct *entry = NULL,*n;
   spin_lock_irqsave(&my_lock, flags);
   list_for_each_entry_safe(entry, n, &my_list, list) {
      printk(KERN_INFO "Node is %d: %d\n", entry->PID, entry->CPUTime);
   }
   spin_unlock_irqrestore(&my_lock, flags);
}

// deletes a node from the linked list
int delete_node(int PID)
{
   unsigned long flags;
   struct ll_struct *entry = NULL, *n;
   // list_for_each_entry(entry, &my_list, list) {
   spin_lock_irqsave(&my_lock, flags);
   list_for_each_entry_safe(entry, n, &my_list, list) {
      if (entry->PID == PID) {
         printk(KERN_INFO "Found the element %d\n",
                entry->PID);
         list_del(&(entry->list));
         kfree(entry);
         return 0;
      }
   }
   spin_unlock_irqrestore(&my_lock, flags);
   printk(KERN_INFO "Could not find the element %d\n", PID);
   return 1;
}

/* -------------------------------------------------------------------------- */
/*                                Work Handler                                */
/* -------------------------------------------------------------------------- */
static void work_handler(struct work_struct *work){
   printk(KERN_INFO "work handler called\n");
   // variables
   unsigned long flags;
   unsigned long cpu_time;
   struct ll_struct *entry = NULL, *n;

   // loop through kernel linked list and update process CPUTimes
   spin_lock_irqsave(&my_lock, flags);
   list_for_each_entry_safe(entry, n, &my_list, list) {
      printk(KERN_INFO "for each list entry");
      if(get_cpu_use(entry->PID, &cpu_time) == 0){
         // update process cpu time
         entry->CPUTime = cpu_time;
      }
      else{
         // remove process from linked list
         // delete_node(entry->PID);
         printk(KERN_INFO "no process");
      }
   }
   spin_unlock_irqrestore(&my_lock, flags);
   

}

/* -------------------------------------------------------------------------- */
/*                                Kernel Timer                                */
/* -------------------------------------------------------------------------- */
void my_timer_callback(struct timer_list *timer) {
   // pr_info("This line is printed every %d ms.\n", time_interval);

   INIT_WORK(&my_work, work_handler);
	queue_work(system_wq, &my_work);

   /* this will make a periodic timer */
   mod_timer(&my_timer, jiffies + msecs_to_jiffies(time_interval));
}

/* -------------------------------------------------------------------------- */
/*                                  Proc File                                 */
/* -------------------------------------------------------------------------- */

//TODO: procfs_write
static ssize_t procfs_write(struct file *file, const char __user *buff, size_t len, loff_t *off)
{
   unsigned long flags;

   /* ------------------------------ lock procfile ----------------------------- */
   spin_lock_irqsave(&my_lock, flags);

   /* Clear internal buffer */
   memset(&procfs_buffer[0], 0, sizeof(procfs_buffer));
   
   procfs_buffer_size = len;
   if (procfs_buffer_size > PROCFS_MAX_SIZE)
      procfs_buffer_size = PROCFS_MAX_SIZE;

   if (copy_from_user(procfs_buffer, buff, procfs_buffer_size))
      return -EFAULT;

   add_node(simple_strtoul(procfs_buffer, NULL, 10));

   procfs_buffer[procfs_buffer_size & (PROCFS_MAX_SIZE - 1)] = '\0';

   /* ----------------------------- unlock procfile ---------------------------- */
   spin_unlock_irqrestore(&my_lock, flags);

   *off += procfs_buffer_size;
   pr_info("procfile write %s\n", procfs_buffer);

   return procfs_buffer_size;
}

//TODO: procfs_read
static ssize_t procfs_read(struct file *file_pointer, char __user *buffer, size_t buffer_length, loff_t *offset)
{
   unsigned long flags;
   struct ll_struct *entry = NULL, *n;
   char* node_string;

   //Clear internal buffer
   spin_lock_irqsave(&my_lock, flags);
   memset(&procfs_buffer[0], 0, sizeof(procfs_buffer)); 
   spin_unlock_irqrestore(&my_lock, flags);

   /* --------------------- Write Process Data to Procfile --------------------- */
   spin_lock_irqsave(&my_lock, flags);
   list_for_each_entry_safe(entry, n, &my_list, list){
      // allocate a string to store the node information
      node_string = kmalloc(2*sizeof(entry), GFP_KERNEL);
      sprintf(node_string, "%d: %d\n", entry->PID, entry->CPUTime);
      // check for buffer overflow
      if(strlen(node_string) + strlen(procfs_buffer) >= PROCFS_MAX_SIZE){
         printk(KERN_INFO "Buffer overflow\n");
      }
      else{
         // add node info to the procfs_buffer
         strcat(procfs_buffer, node_string);
      }
      // deallocate string
      kfree(node_string);
   }
   spin_unlock_irqrestore(&my_lock, flags);
   /* -------------------------------------------------------------------------- */

   int len = sizeof(procfs_buffer);
   ssize_t ret = len;
   
   //pr_info("len is %d, offest is %d\n", (int)len, (int)*offset);

   if (*offset >= len) {
      return 0;
   }

   if (copy_to_user(buffer, procfs_buffer, len)) {
      pr_info("copy_to_user failed\n");
      ret = 0;
   } else {
      pr_info("procfile read /proc/kmlab/%s\n", PROC_FILE_NAME);
      *offset += len;
   }

   return ret;
}

//TODO: The proc_ops
static struct proc_ops proc_fops = { 
   .proc_read = procfs_read, 
   .proc_write = procfs_write, 
   // .proc_open = procfs_open, 
   // .proc_release = procfs_close, 
}; 



/* -------------------------------------------------------------------------- */
/*                            Module Init and Exit                            */
/* -------------------------------------------------------------------------- */

// kmlab_init - Called when module is loaded
int __init kmlab_init(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE LOADING\n");
   #endif
   // Insert your code here ...

   // init the kernel linked list
   INIT_LIST_HEAD(&my_list);

   // create the proc directory and file
   proc_dir = proc_mkdir(PROC_DIR_NAME, NULL);
   proc_file = proc_create(PROC_FILE_NAME, 0666, proc_dir, &proc_fops);
   if (NULL == proc_file) { 
      pr_alert("Error:Could not initialize /proc/%s\n", PROC_FILE_NAME); 
      return -ENOMEM; 
    } 

   // initialize timer
   timer_setup(&my_timer, my_timer_callback, 0);
   mod_timer(&my_timer, jiffies + msecs_to_jiffies(time_interval));
   
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
   // remove the proc directory
   proc_remove(proc_dir);

   // remove timer
   del_timer(&my_timer);
   

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(kmlab_init);
module_exit(kmlab_exit);
