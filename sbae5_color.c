#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Igor Kobiakov");
MODULE_DESCRIPTION("SB AE-5 LED color control");

#define MMIO_BASE_ADDR 0xf4600000
#define REGION_SIZE 0x1024

static void __iomem *mmio_base;

#define PROC_FILENAME "sbae5_led"

static void trigger_led_sequence(uint32_t led_color)
{
    int i, j, x;

    if (!mmio_base) {
        pr_err("SBAE_LED_DRIVER: MMIO base not mapped\n");
        return;
    }

    iowrite32(0xFF, mmio_base + 0x304);
    iowrite32(0xFF, mmio_base + 0x304);

    for (i = 0; i < 32; i++) {
        iowrite32(0x02, mmio_base + 0x320);
        iowrite32(0x103, mmio_base + 0x320);
        iowrite32(0x03, mmio_base + 0x320);
    }

    for (x = 0; x < 2; x++) {
        for (j = 0; j < 5; j++) {
            for (i = 0; i < 8; i++) {
                iowrite32(0x102, mmio_base + 0x320);
                iowrite32(0x103, mmio_base + 0x320);
                iowrite32(0x03, mmio_base + 0x320);
            }

            for (i = 0; i < 24; i++) {
                uint32_t bit = (led_color >> (23 - i)) & 0x01;

                iowrite32((bit == 1) ? 0x102 : 0x02, mmio_base + 0x320);
                iowrite32(0x103, mmio_base + 0x320);
                iowrite32(0x03, mmio_base + 0x320);
            }
        }
    }
}

static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
    char kbuf[10];
    int ret;
    uint32_t new_color;

    if (count > sizeof(kbuf) - 1) {
        pr_warn("SBAE_LED_DRIVER: Input too long\n");
        return -EINVAL;
    }

    if (copy_from_user(kbuf, buffer, count)) {
        return -EFAULT;
    }
    kbuf[count] = '\0';

    ret = kstrtouint(kbuf, 16, &new_color);
    if (ret) {
        return ret;
    }

    trigger_led_sequence(new_color);

    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_write = proc_write,
};

static int __init led_driver_init(void)
{
    mmio_base = ioremap(MMIO_BASE_ADDR, REGION_SIZE);
    if (!mmio_base) {
        pr_err("SBAE_LED_DRIVER: Failed to remap MMIO memory\n");
        return -ENOMEM;
    }

    if (proc_create(PROC_FILENAME, 0666, NULL, &proc_file_ops) == NULL) {
        pr_err("SBAE_LED_DRIVER: Failed to create /proc/%s\n", PROC_FILENAME);
        iounmap(mmio_base);
        return -ENOMEM;
    }
    pr_info("SBAE_LED_DRIVER: Created /proc/%s entry\n", PROC_FILENAME);
    pr_info("SBAE_LED_DRIVER: To set color, run: echo \"BBGGRR\" > /proc/%s\n", PROC_FILENAME);

    return 0;
}

static void __exit led_driver_exit(void)
{
    remove_proc_entry(PROC_FILENAME, NULL);

    if (mmio_base) {
        iounmap(mmio_base);
    }
}

module_init(led_driver_init);
module_exit(led_driver_exit);
