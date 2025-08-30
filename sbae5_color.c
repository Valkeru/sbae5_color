#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/sched.h>

#define MMIO_BASE_ADDR 0xf4600000
#define REGION_SIZE 0x1024

static void __iomem *mmio_base;

static inline void write_mmio(void __iomem *base, uint32_t offset, uint32_t value) {
    iowrite32(value, base + offset);
}

static int __init my_module_init(void) {
    uint32_t hex_value;
    int i, j, x;

    mmio_base = ioremap(MMIO_BASE_ADDR, REGION_SIZE);
    if (!mmio_base) {
        pr_err("Failed to remap MMIO\n");
        return -ENOMEM;
    }

    write_mmio(mmio_base, 0x304, 0xFF);
    write_mmio(mmio_base, 0x304, 0xFF);

    for (i = 0; i < 32; i++) {
        write_mmio(mmio_base, 0x320, 0x02);
        write_mmio(mmio_base, 0x320, 0x103);
        write_mmio(mmio_base, 0x320, 0x03);
    }

    for (x = 0; x < 2; x++) {
        for (j = 0; j < 5; j++) {
            for (i = 0; i < 8; i++) {
                write_mmio(mmio_base, 0x320, 0x102);
                write_mmio(mmio_base, 0x320, 0x103);
                write_mmio(mmio_base, 0x320, 0x03);
            }

            hex_value = 0x00FF00;

            for (i = 0; i < 24; i++) {
                uint32_t bit = (hex_value >> (23 - i)) & 0x01;
                write_mmio(mmio_base, 0x320, (bit == 1) ? 0x102 : 0x02);
                write_mmio(mmio_base, 0x320, 0x103);
                write_mmio(mmio_base, 0x320, 0x03);
            }
        }
    }

    return 0;
}

static void __exit my_module_exit(void) {
    if (mmio_base)
        iounmap(mmio_base);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Valkeru Fox");
MODULE_DESCRIPTION("Change SB AE-5 color");
