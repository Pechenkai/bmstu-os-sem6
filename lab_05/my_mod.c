#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Panteleev Vasiliy");
MODULE_DESCRIPTION("Print info about task");
MODULE_VERSION("Version 1.0");

static int __init my_init(void) {
    struct task_struct *task = &init_task;
    
    do {
        printk(KERN_INFO "+ comm - %s, pid - %d, parent - %s, ppid - %d, state - %d, state_ps - %c, on_cpu - %d, flags - %x, prio - %d, policy - %u",
               task->comm,
               task->pid,
               task->parent->comm,
               task->parent->pid,
               task->__state,
               task_state_to_char(task),
               task->on_cpu,
               task->flags,
               task->prio,
               task->policy);

        struct thread_info *ti = task_thread_info(task);
        printk("CPU: %d\n", ti->cpu);
    } while ((task = next_task(task)) != &init_task);

    printk(KERN_INFO "+ Current task: comm - %s, pid - %d, parent - %s, ppid - %d, state - %d, state_ps - %c, on_cpu - %d, flags - %x, prio - %d, policy - %u\n",
           current->comm,
           current->pid,
           current->parent->comm,
           current->parent->pid,
           current->__state,
           task_state_to_char(current),
           current->on_cpu,
           current->flags,
           current->prio,
           current->policy);
    return 0;
}

static void __exit my_exit(void) {
    printk(KERN_INFO "See u!\n");
}

module_init(my_init);
module_exit(my_exit);
