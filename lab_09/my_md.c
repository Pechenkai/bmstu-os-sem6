#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#define DIRNAME "sdir"
#define FILENAME "single"
#define SYMLINK_NAME "symlinksingle"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Panteleev VM");

static struct proc_dir_entry *dir = NULL;
static struct proc_dir_entry *file = NULL;
static struct proc_dir_entry *link = NULL;

#define MAX_PIDS 32

static int pids[MAX_PIDS];
static int pid_count = 0;

static int my_seq_show(struct seq_file *s, void *v);
static ssize_t my_proc_write(struct file *file, const char __user *buf,
                             size_t count, loff_t *ppos);

static int my_proc_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "--- my open()\n");
    return single_open(file, my_seq_show, NULL);
}

static ssize_t my_proc_read(struct file *f, char __user *c, size_t s,
                            loff_t *off) {
    printk(KERN_INFO "--- my read()\n");
    return seq_read(f, c, s, off);
}

static int my_proc_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "--- my release()\n");
    return single_release(inode, file);
}

static const struct proc_ops file_ops = {
    .proc_open = my_proc_open,
    .proc_read = my_proc_read,
    .proc_write = my_proc_write,
    .proc_release = my_proc_release,
};

static int my_seq_show(struct seq_file *s, void *v) {
    int i;
    printk(KERN_INFO "--- my show() m=%p v=%p", s, v);
    for (i = 0; i < pid_count; i++) {
        int pid = pids[i];
        printk(KERN_INFO "PID %d \n", pid);

        struct task_struct *task = pid_task(find_vpid(pid), PIDTYPE_PID);
        if (!task) {
        seq_printf(s, "--- Process with PID %d not found\n", pid);
        }

        seq_printf(s,
                "Comm: %s\n"
                "PID: %d\n"
                "PComm: %s\n"
                "PPID: %d\n"
                "State: %x\n"
                "Flags: %x\n"
                "Prio: %d\n"
                "Static prio: %d\n"
                "Normal prio: %d\n"
                "Policy: %d\n"
                "User time: %llu\n"
                "System time: %llu\n---------------\n",
                task->comm, task->pid, task->real_parent->comm,
                task->real_parent->pid, task->__state, task->flags, task->prio,
                task->static_prio, task->normal_prio, task->policy, task->utime,
                task->stime);
    }

    return 0;
}
static ssize_t my_proc_write(struct file *file, const char __user *buf,
                             size_t count, loff_t *ppos) {
    printk(KERN_INFO "--- my write()\n");
    char temp_buf[16] = {0};
    int new_pid;
    int ret;

    if (count >= sizeof(temp_buf))
        return -EINVAL;

    if (copy_from_user(temp_buf, buf, count))
        return -EFAULT;

    temp_buf[count] = '\0';

    ret = kstrtoint(temp_buf, 10, &new_pid);
    if (ret != 0) {
        printk(KERN_INFO "--- Invalid PID\n");
        return -EINVAL;
    }

    pids[pid_count++] = new_pid;

    printk(KERN_INFO "PID: %d\n", new_pid);

    return count;
}

static int __init single_init(void) {
    printk(KERN_INFO "--- Module loaded\n");

    dir = proc_mkdir(DIRNAME, NULL);
    if (!dir) {
        printk(KERN_ERR "--- Failed to create directory\n");
        return -ENOMEM;
    }

    file = proc_create(FILENAME, S_IRUGO | S_IWUGO, dir, &file_ops);
    if (!file) {
        printk(KERN_ERR "--- Failed to create file\n");
        return -ENOMEM;
    }

    link = proc_symlink(SYMLINK_NAME, NULL, "/proc/" DIRNAME "/" FILENAME);
    if (!link) {
        printk(KERN_ERR "--- Failed to create symlink\n");
        return -ENOMEM;
    }

    return 0;
}

static void __exit single_exit(void) {
    remove_proc_entry(SYMLINK_NAME, NULL);
    remove_proc_entry(FILENAME, dir);
    remove_proc_entry(DIRNAME, NULL);

    printk(KERN_INFO "--- Module unloaded\n");
}

module_init(single_init);
module_exit(single_exit);