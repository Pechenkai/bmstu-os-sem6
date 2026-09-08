#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/time.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");

#define IRQ_NUM 1
#define DIR_NAME "key_buf"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

typedef struct
{
    struct work_struct work;
    ktime_t start_time;
    int code;
} key_work_t;

static struct workqueue_struct *key_wq;
static key_work_t *work1, *work2;

static int left_shift_pressed = 0;
static int right_shift_pressed = 0;
static int caps_lock_active = 0;

static char *key_names_lowercase[84] = {
    " ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace",
    "Tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "Enter", "Ctrl",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`", "Shift (left)", "\\",
    "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "Shift (right)",
    "*", "Alt", "Space", "CapsLock",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
    " ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"};

static char *key_names_uppercase[84] = {
    " ", "Esc", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "Backspace",
    "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "Enter", "Ctrl",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "~", "Shift (left)", "|",
    "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Shift (right)",
    "*", "Alt", "Space", "CapsLock",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
    " ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"};

static struct proc_dir_entry *proc_file;

static int key_code = -1;
static char *key_name;

static int should_use_uppercase(void)
{
    return (left_shift_pressed || right_shift_pressed) ^ caps_lock_active;
}

static int my_show(struct seq_file *m, void *v)
{
    if (key_code != -1)
        seq_printf(m, "Key: %s Code: %d\n", key_name, key_code);
    return 0;
}

static int my_open(struct inode *inode, struct file *file)
{
    return single_open(file, my_show, NULL);
}

static struct proc_ops key_fops = {
    .proc_read = seq_read,
    .proc_open = my_open,
    .proc_release = single_release};

static void work_fun2(struct work_struct *work)
{

    key_work_t *key_work = (key_work_t *)work;
    int code = key_work->code;
    int is_release = (code & 0x80) ? 1 : 0;
    int press_code = code & 0x7F;

    ktime_t end_time;
    s64 diff_ns;

    switch (code)
    {
    case 0x2A: // Left Shift press
        left_shift_pressed = 1;
        return;
    case 0xAA: // Left Shift release
        left_shift_pressed = 0;
        return;
    case 0x36: // Right Shift press
        right_shift_pressed = 1;
        return;
    case 0xB6: // Right Shift release
        right_shift_pressed = 0;
        return;
    case 0x3A: // Caps Lock press
        caps_lock_active = !caps_lock_active;
        return;
    }

    if (!is_release)
    {
        return;
    }

    if (press_code < 84 && press_code != 28)
    {
        printk(KERN_INFO "work 1 : begin");

        key_code = press_code;
        end_time = ktime_get();
        diff_ns = ktime_to_ns(ktime_sub(end_time, key_work->start_time));

        if (should_use_uppercase())
        {
            key_name = key_names_uppercase[press_code];
        }
        else
        {
            key_name = key_names_lowercase[press_code];
        }

        printk(KERN_INFO "work 1 : Key: %s Code: %d Time: %lld\n", key_name, key_code, diff_ns);
    }
    printk(KERN_INFO "work 1 : end");
}


static void work_fun1(struct work_struct *work)
{
    key_work_t *key_work = (key_work_t *)work;
    int code = key_work->code;
    int is_release = (code & 0x80) ? 1 : 0;

    switch (code)
    {
    case 0x2A: // Left Shift press
        left_shift_pressed = 1;
        return;
    case 0xAA: // Left Shift release
        left_shift_pressed = 0;
        return;
    case 0x36: // Right Shift press
        right_shift_pressed = 1;
        return;
    case 0xB6: // Right Shift release
        right_shift_pressed = 0;
        return;
    case 0x3A: // Caps Lock press
        caps_lock_active = !caps_lock_active;
        return;
    }

    if (!is_release)
    {
        return;
    }

    printk(KERN_INFO "work 2 : sleep start");
    fsleep(3);
    printk(KERN_INFO "work 2 : sleep end");
    printk(KERN_INFO "work 2 : begin");

    int press_code = code & 0x7F;
    ktime_t end_time;
    s64 diff_ns;

    if (press_code < 84 && press_code != 28)
    {
        key_code = press_code;
        end_time = ktime_get();
        diff_ns = ktime_to_ns(ktime_sub(end_time, key_work->start_time));

        if (should_use_uppercase())
        {
            key_name = key_names_uppercase[press_code];
        }
        else
        {
            key_name = key_names_lowercase[press_code];
        }

        printk(KERN_INFO "work 2 : Key: %s Code: %d Time: %lld\n", key_name, key_code, diff_ns);
    }
    printk(KERN_INFO "work 2 : end");
}

static irqreturn_t interrupt_handler(int irq, void *dev_id)
{
    if (irq == IRQ_NUM)
    {
        int code = inb(0x60);

        work1->start_time = ktime_get();
        work1->code = code;
        work2->code = code;

        queue_work(key_wq, (struct work_struct *)work1);
        queue_work(key_wq, (struct work_struct *)work2);

        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

static int __init my_init(void)
{
    int ret = request_irq(IRQ_NUM, interrupt_handler, IRQF_SHARED, "interrupt_handler_wq", (void *)(interrupt_handler));
    if (ret)
    {
        printk(KERN_ERR "Error: request_irq: %d\n", ret);
        return ret;
    }

    key_wq = alloc_workqueue("my_workqueue", __WQ_LEGACY | WQ_MEM_RECLAIM, 1);
    if (!key_wq)
    {
        free_irq(IRQ_NUM, (void *)(interrupt_handler));
        printk(KERN_ERR "Error: Failed to create workqueue\n");
        return -ENOMEM;
    }

    work1 = (key_work_t *)kmalloc(sizeof(key_work_t), GFP_KERNEL);
    if (!work1)
    {
        free_irq(IRQ_NUM, (void *)(interrupt_handler));
        destroy_workqueue(key_wq);
        printk(KERN_ERR "Error: Failed to allocate immediate work\n");
        return -ENOMEM;
    }
    INIT_WORK((struct work_struct *)work1, work_fun2);

    work2 = (key_work_t *)kmalloc(sizeof(key_work_t), GFP_KERNEL);
    if (!work2)
    {
        free_irq(IRQ_NUM, (void *)(interrupt_handler));
        destroy_workqueue(key_wq);
        kfree(work1);
        printk(KERN_ERR "ERROR: Failed to allocate delayed work\n");
        return -ENOMEM;
    }
    INIT_WORK((struct work_struct *)work2, work_fun1);

    proc_file = proc_create("key_buf_wq", 0444, NULL, &key_fops);
    if (!proc_file)
    {
        printk(KERN_ERR "ERROR: Failed to create proc file\n");
        free_irq(IRQ_NUM, (void *)(interrupt_handler));
        destroy_workqueue(key_wq);
        kfree(work1);
        kfree(work2);
        return -ENOMEM;
    }

    printk(KERN_INFO "Keyboard module loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    proc_remove(proc_file);

    flush_workqueue(key_wq);
    destroy_workqueue(key_wq);

    free_irq(IRQ_NUM, (void *)(interrupt_handler));

    kfree(work1);
    kfree(work2);

    printk(KERN_INFO "INFO: Keyboard module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);