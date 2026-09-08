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
#include <linux/version.h>
#include <linux/time.h>
#include <asm/io.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");

#define IRQ_NO 1
#define BUF_SIZE 128

static const char *lowercase[] = {
    "None", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
    "Backspace", "Tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]",
    "Enter", "Left Ctrl", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`",
    "Left Shift", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/",
    "Right Shift", "Keypad *", "Left Alt", "Space", "Caps Lock",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10"};

static const char *uppercase[] = {
    "None", "Esc", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+",
    "Backspace", "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}",
    "Enter", "Left Ctrl", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "~",
    "Left Shift", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?",
    "Right Shift", "Keypad *", "Left Alt", "Space", "Caps Lock",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10"};

static struct tasklet_struct *tasklet = NULL;
static char buffer[BUF_SIZE];
static ktime_t start_time;
static u64 total_time = 0;

static int left_shift_pressed = 0;
static int right_shift_pressed = 0;
static int caps_lock_active = 0;

static int my_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Last pressed key: %s\n", buffer);
    seq_printf(m, "Time: %llu ns\n", total_time);

    return 0;
}

static int my_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, my_proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open = my_proc_open,
    .proc_read = seq_read,
    .proc_release = single_release,
};

static int should_use_uppercase(void)
{
    return (left_shift_pressed || right_shift_pressed) ^ caps_lock_active;
}

void my_tasklet_fun(unsigned long data)
{
    int code = inb(0x60);
    char *key_event;
    const char **current_layout = should_use_uppercase() ? uppercase : lowercase;

    switch (code)
    {
    case 0x2A:
        left_shift_pressed = 1;
        return;
    case 0xAA:
        left_shift_pressed = 0;
        return;
    case 0x36:
        right_shift_pressed = 1;
        return;
    case 0xB6:
        right_shift_pressed = 0;
        return;
    }

    if ((code >= 0x47 && code <= 0x53) || code == 0x1C)
    {
        total_time = ktime_to_ns(ktime_sub(ktime_get(), start_time));
    }
    else if (code & 0x80)
    {
        total_time = ktime_to_ns(ktime_sub(ktime_get(), start_time));
    }
    else
    {
        key_event = "pressed";
        code &= 0x7F;
        if (code >= 0 && code < sizeof(lowercase) / sizeof(lowercase[0]))
        {
            printk(KERN_INFO "Tasklet : Key %s: %s (code=0x%02x), Time=%lld ns\n",
                   key_event, current_layout[code], code, ktime_to_ns(ktime_sub(ktime_get(), start_time)));
            snprintf(buffer, sizeof(buffer), "%s (code=0x%02x)", current_layout[code], code);
        }

        total_time = ktime_to_ns(ktime_sub(ktime_get(), start_time));
    }
}

static irqreturn_t interrupt_handler(int irq, void *dev_id)
{
    if (irq == IRQ_NO)
    {
        start_time = ktime_get();
        tasklet_schedule(tasklet);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

static int __init my_init(void)
{
    if (proc_create("keyboard", 0, NULL, &proc_fops) == NULL)
    {
        return -ENOMEM;
    }
    int ret = request_irq(IRQ_NO, interrupt_handler, IRQF_SHARED, "interrupt_handler_tasklet", (void *)(interrupt_handler));
    if (ret)
    {
        return ret;
    }

    buffer[0] = '\0';
    tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
    if (!tasklet)
    {
        free_irq(IRQ_NO, (void *)(interrupt_handler));
        return -ENOMEM;
    }
    tasklet_init(tasklet, my_tasklet_fun, 0);

    return 0;
}

static void __exit my_exit(void)
{
    remove_proc_entry("keyboard", NULL);
    tasklet_kill(tasklet);
    kfree(tasklet);
    free_irq(IRQ_NO, (void *)(interrupt_handler));
}

module_init(my_init);
module_exit(my_exit);