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
#include <asm/io.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

#define IRQ_NO 1

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

static int   t_key_code = -1;
static const char *t_key_name;

static int left_shift_pressed = 0;
static int right_shift_pressed = 0;
static int caps_lock_active = 0;

static int my_proc_show(struct seq_file *m, void *v)
{
    if (t_key_code != -1)
        seq_printf(m, "Key: %s Code: %d\n", t_key_name, t_key_code);
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
    int code = (int)data;
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

        const char * const *layout = should_use_uppercase()
            ? uppercase : lowercase;

        t_key_code = press_code;
        t_key_name = layout[press_code];

        pr_info("tasklet: Key: %s Code: %d\n", t_key_name, t_key_code);
    }
}

static irqreturn_t interrupt_handler(int irq, void *dev_id)
{
    if (irq == IRQ_NO) {
        int scancode = inb(0x60);
        if (tasklet) {
            tasklet->data = (unsigned long)scancode;
            tasklet_schedule(tasklet);
        }
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

static int __init my_init(void)
{
    if (proc_create("ts", 0, NULL, &proc_fops) == NULL)
        return -ENOMEM;
    
    int ret = request_irq(IRQ_NO, interrupt_handler, IRQF_SHARED,
                          "tasklet_ih", (void *)(interrupt_handler));
    if (ret)
        return ret;

    tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
    if (!tasklet) {
        free_irq(IRQ_NO, (void *)(interrupt_handler));
        return -ENOMEM;
    }
    tasklet_init(tasklet, my_tasklet_fun, 0);

    return 0;
}

static void __exit my_exit(void)
{
    remove_proc_entry("ts", NULL);
    tasklet_kill(tasklet);
    kfree(tasklet);
    free_irq(IRQ_NO, (void *)(interrupt_handler));
}

module_init(my_init);
module_exit(my_exit);