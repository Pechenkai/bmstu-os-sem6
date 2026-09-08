#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>

#define DIRNAME "sdir"
#define FILENAME "inf"
#define SYMLINK_NAME "symlinkinf"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Panteleev VM");
MODULE_DESCRIPTION("Kernel module to get task_struct info by PID");
MODULE_VERSION("1.0");

static struct proc_dir_entry *proc_dir, *proc_file, *proc_sl;

static int pid = -1;
static char output_buf[1024];

static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    struct task_struct *task;
    int len;
    printk(KERN_INFO "fortune: read\n");

    if (pid == -1) 
    {
        len = snprintf(output_buf, sizeof(output_buf), "PID not set.\n");
        if (*ppos > 0) return 0;
        if (copy_to_user(ubuf, output_buf, len)) return -EFAULT;
            *ppos = len;
        return len;
    }

    if (*ppos > 0)
        return 0;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        len = snprintf(output_buf, sizeof(output_buf), "No process with PID %d found.\n", pid);
        if (copy_to_user(ubuf, output_buf, len)) return -EFAULT;
            *ppos = len;
        return len;
    }

    len = snprintf(output_buf, sizeof(output_buf),
        "PID: %d\n"
        "Parent: %d\n"
        "Name: %s\n"
        "Policy: %#x\n"
        "State: %#x\n"
        "Flags: %#x\n"
        "Priority: %d\n"
        "Static Priority: %d\n"
        "Normal Priority: %d\n"
        "RT Priority: %d\n",
        task->pid,
        task->parent->pid,
        task->comm,
        task->policy,
        task->__state,
        task->flags,
        task->prio,
        task->static_prio,
        task->normal_prio,
        task->rt_priority
        );

    if (copy_to_user(ubuf, output_buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static ssize_t proc_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
    char buf[16];
    printk(KERN_INFO "fortune: write\n");

    if (copy_from_user(buf, ubuf, min(count, sizeof(buf) - 1)))
        return -EFAULT;

    buf[min(count, sizeof(buf) - 1)] = '\0';
    if (kstrtoint(buf, 10, &pid))
        return -EINVAL;

    return count;
}

static int proc_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "fortune: open\n");
    return 0;
}

static int proc_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "fortune: release\n");
    return 0;
}

static struct proc_ops proc_file_ops = {
    .proc_open = proc_open,
    .proc_read  = proc_read,
    .proc_write = proc_write,
    .proc_release = proc_release
};

static int __init my_module_init(void) {
    printk(KERN_INFO "fortune: init\n");
    proc_dir = proc_mkdir(DIRNAME, NULL);
    if (!proc_dir) {
        printk(KERN_ERR "Failed to create subdir");
        return -ENOMEM;
    }

    proc_file = proc_create(FILENAME, 0666, proc_dir, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create virtual file\n");
        proc_remove(proc_dir);
        return -ENOMEM;
    }

    proc_sl = proc_symlink(SYMLINK_NAME, NULL, DIRNAME "/" FILENAME);
    if (!proc_sl) {
        printk(KERN_ERR "Failed to create symlink\n");
        proc_remove(proc_file);
        proc_remove(proc_dir);
        return -ENOMEM;
    }

    printk(KERN_INFO "fortune: module loaded\n");
    return 0;
}

static void __exit my_module_exit(void) {
    proc_remove(proc_sl);
    proc_remove(proc_file);
    proc_remove(proc_dir);
    printk(KERN_INFO "fortune: module unloaded.\n");
}

module_init(my_module_init);
module_exit(my_module_exit);