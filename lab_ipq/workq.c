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
#include <asm/io.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");

#define IRQ_NUM 1

typedef struct {
    struct work_struct work;
    int code;
} key_work_t;

static struct workqueue_struct *key_wq;
static key_work_t *work1, *work2;

static int left_shift_pressed  = 0;
static int right_shift_pressed = 0;
static int caps_lock_active    = 0;

static const char * const key_names_lowercase[84] = {
    " ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace",
    "Tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "Enter", "Ctrl",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`", "Shift (left)", "\\",
    "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "Shift (right)",
    "*", "Alt", "Space", "CapsLock",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
    " ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"
};

static const char * const key_names_uppercase[84] = {
    " ", "Esc", "!", "@", "#", "$", "/", "^", "&", "*", "(", ")", "_", "+", "Backspace",
    "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "Enter", "Ctrl",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "~", "Shift (left)", "|",
    "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Shift (right)",
    "*", "Alt", "Space", "CapsLock",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
    " ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"
};

static struct proc_dir_entry *proc_file;
static int   key_code = -1;
static const char *key_name;

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

static const struct proc_ops key_fops = {
    .proc_read    = seq_read,
    .proc_open    = my_open,
    .proc_release = single_release,
};

static void work_fun_fast(struct work_struct *work)
{
    key_work_t *kw = container_of(work, key_work_t, work);
    int code = kw->code;
    int is_release = code & 0x80;
    int press_code = code & 0x7F;

    switch (code) {
    case 0x2A: left_shift_pressed  = 1; return; 
    case 0xAA: left_shift_pressed  = 0; return; 
    case 0x36: right_shift_pressed = 1; return;
    case 0xB6: right_shift_pressed = 0; return; 
    case 0x3A: caps_lock_active   ^= 1; return; 
    }

    if (!is_release)
        return; 

    if (press_code < 84 && press_code != 28 && press_code != 72 &&
        press_code != 80 && press_code != 77 && press_code != 75) {

        key_code = press_code;
        key_name = should_use_uppercase()
            ? key_names_uppercase[press_code]
            : key_names_lowercase[press_code];

        pr_info("work fast: Key: %s Code: %d\n", key_name, key_code);
    }
}

static void work_fun_slow(struct work_struct *work)
{
    key_work_t *kw = container_of(work, key_work_t, work);
    int code = kw->code;
    int is_release = !!(code & 0x80);
    int press_code = code & 0x7F;

    switch (code) {
    case 0x2A: left_shift_pressed  = 1; return;
    case 0xAA: left_shift_pressed  = 0; return;
    case 0x36: right_shift_pressed = 1; return;
    case 0xB6: right_shift_pressed = 0; return;
    case 0x3A: caps_lock_active   ^= 1; return;
    }

    if (!is_release)
        return;

    if (press_code < 84 && press_code != 28 && press_code != 72 &&
        press_code != 80 && press_code != 77 && press_code != 75) {

        pr_info("work slow: sleep start");
        msleep(10);
        pr_info("work slow: sleep end");

        key_code = press_code;
        key_name = should_use_uppercase()
            ? key_names_uppercase[press_code]
            : key_names_lowercase[press_code];

        pr_info("work slow: Key: %s Code: %d\n", key_name, key_code);
    }
}

static irqreturn_t interrupt_handler(int irq, void *dev_id)
{
    if (irq != IRQ_NUM)
        return IRQ_NONE;

    {
        int code = inb(0x60);

        work1->code = code;
        work2->code = code;

        queue_work(key_wq, &work1->work);
        queue_work(key_wq, &work2->work);
    }
    return IRQ_HANDLED;
}

static int __init my_init(void)
{
    int ret;

    ret = request_irq(IRQ_NUM, interrupt_handler, IRQF_SHARED,
                      "workqueue_ih", (void *)interrupt_handler);
    if (ret) {
        pr_err("request_irq failed: %d\n", ret);
        return ret;
    }

    key_wq = alloc_workqueue("my_workqueue", WQ_MEM_RECLAIM | WQ_UNBOUND, 1);
    if (!key_wq) {
        pr_err("alloc_workqueue failed\n");
        free_irq(IRQ_NUM, (void *)interrupt_handler);
        return -ENOMEM;
    }

    work1 = kmalloc(sizeof(*work1), GFP_KERNEL);
    if (!work1) {
        pr_err("kmalloc work1 failed\n");
        destroy_workqueue(key_wq);
        free_irq(IRQ_NUM, (void *)interrupt_handler);
        return -ENOMEM;
    }
    INIT_WORK(&work1->work, work_fun_fast);

    work2 = kmalloc(sizeof(*work2), GFP_KERNEL);
    if (!work2) {
        pr_err("kmalloc work2 failed\n");
        kfree(work1);
        destroy_workqueue(key_wq);
        free_irq(IRQ_NUM, (void *)interrupt_handler);
        return -ENOMEM;
    }
    INIT_WORK(&work2->work, work_fun_slow);

    proc_file = proc_create("wq", 0444, NULL, &key_fops);
    if (!proc_file) {
        pr_err("proc_create failed\n");
        kfree(work2);
        kfree(work1);
        destroy_workqueue(key_wq);
        free_irq(IRQ_NUM, (void *)interrupt_handler);
        return -ENOMEM;
    }

    pr_info("Keyboard module loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    proc_remove(proc_file);
    flush_workqueue(key_wq);
    destroy_workqueue(key_wq);
    free_irq(IRQ_NUM, (void *)interrupt_handler);
    kfree(work1);
    kfree(work2);
    pr_info("Keyboard module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);