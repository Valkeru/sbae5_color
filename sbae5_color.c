#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/pci.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Igor Kobiakov");
MODULE_DESCRIPTION("SB AE-5 LED color control");

#define REGION_SIZE 0x1024
#define LED_CONTROL_OFFSET 0x320
#define TARGET_MEM_REGION 2
#define DRIVER_NAME "SBAE5_LED_DRIVER"

// Определяем идентификаторы устройства
#define CREATIVE_VENDOR_ID 0x1102
#define AE5_DEVICE_ID      0x0012

// Определяем Subsystem IDs для разных моделей
#define SUBSYS_VENDOR_ID   0x1102
#define SUBSYS_AE5_DEVICE_ID      0x0051
#define SUBSYS_AE5_PLUS_DEVICE_ID 0x0191


// Mapped memory area pointer
static void __iomem *mmio_base;

unsigned long long base_address = 0;

// Use two file names to be able to detect particular device in OpenRGB
#define AE5_PROC_FILENAME "sbae5_led"
#define AE5_PLUS_PROC_FILENAME "sbae5_plus_led"

const char* proc_filename;



/**
 * LED control color
 * @param led_color Led color code to set
 */
static void set_led_color(uint32_t led_color)
{
    if (!mmio_base) {
        pr_err("%s: MMIO base not mapped", DRIVER_NAME);
        return;
    }

    iowrite32(0xFF, mmio_base + 0x304);
    iowrite32(0xFF, mmio_base + 0x304);

    // Send start frame - 32 0's
    for (int i = 0; i < 32; i++) {
        iowrite32(0x02, mmio_base + LED_CONTROL_OFFSET);
        iowrite32(0x103, mmio_base + LED_CONTROL_OFFSET);
        iowrite32(0x03, mmio_base + LED_CONTROL_OFFSET);
    }

    // Setting color, two times to complete color change
    for (int x = 0; x < 2; x++) {

        // 5 LEDs, 32 bit each
        for (int j = 0; j < 5; j++) {
            // Set first 8 bits to 1 (three 1 bits + 5 luma bits)
            for (int i = 0; i < 8; i++) {
                iowrite32(0x102, mmio_base + LED_CONTROL_OFFSET);
                iowrite32(0x103, mmio_base + LED_CONTROL_OFFSET);
                iowrite32(0x03, mmio_base + LED_CONTROL_OFFSET);
            }

            for (int i = 0; i < 24; i++) {
                // Current bit
                uint32_t bit = (led_color >> (23 - i)) & 0x01;

                // Send bit, set the data bit if bit is 1
                iowrite32((bit == 1) ? 0x102 : 0x02, mmio_base + LED_CONTROL_OFFSET);
                // Clock the data bit
                iowrite32(0x103, mmio_base + LED_CONTROL_OFFSET);
                // Toggle the clock
                iowrite32(0x03, mmio_base + LED_CONTROL_OFFSET);
            }
        }
    }
}

static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
    const int color_buffer_size = 10;
    char color_buffer[color_buffer_size];
    uint32_t new_color;

    if (count > color_buffer_size - 1) {
        pr_warn("%s: Input too long", DRIVER_NAME);
        return -EINVAL;
    }

    if (copy_from_user(color_buffer, buffer, count)) {
        return -EFAULT;
    }
    color_buffer[count] = '\0'; // Terminate string with null-character

    // Transform string into number
    int control = kstrtouint(color_buffer, 16, &new_color);
    if (control) {
        return control;
    }

    set_led_color(new_color);

    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_write = proc_write,
};

static int __init led_driver_init(void)
{
    struct pci_dev *pdev = NULL;
    bool device_found = false;
    resource_size_t base_address;

    pr_info("%s: Searching for Creative AE-5 devices", DRIVER_NAME);

    // Используем макрос для безопасного перебора всех PCI устройств в системе
    for_each_pci_dev(pdev) {
        // Проверяем, совпадает ли Vendor и Device ID
        if (pdev->vendor == CREATIVE_VENDOR_ID && pdev->device == AE5_DEVICE_ID) {
            const char *device_name;

            device_found = true;

            // Определяем точное название модели по subsystem ID
            if (pdev->subsystem_vendor == SUBSYS_VENDOR_ID && pdev->subsystem_device == SUBSYS_AE5_PLUS_DEVICE_ID) {
                device_name = "Creative SoundBlaster AE-5 Plus";
                proc_filename = AE5_PLUS_PROC_FILENAME;
            } else if (pdev->subsystem_vendor == SUBSYS_VENDOR_ID && pdev->subsystem_device == SUBSYS_AE5_DEVICE_ID) {
                device_name = "Creative SoundBlaster AE-5";
                proc_filename = AE5_PROC_FILENAME;
            } else {
                device_name = "Creative SoundBlaster AE-5 (Unknown Subsystem)";
            }

            pr_info("%s: Found device: %s at PCI location %s", DRIVER_NAME, device_name, pci_name(pdev));

            // Получаем физический адрес начала региона памяти BAR 2
            // Для этой операции не требуется pci_enable_device() или pci_request_regions()
            base_address = pci_resource_start(pdev, TARGET_MEM_REGION);

            if (base_address) {
                // Выводим найденный адрес в лог ядра.
                // %pa - специальный формат для вывода физических адресов.
                pr_info("%s: Physical Base Address of BAR %d is: 0x%pa\n",
                        DRIVER_NAME, TARGET_MEM_REGION, &base_address);
                break;
            } else {
                pr_warn("%s: Device found, but BAR %d is not available or has a zero address", DRIVER_NAME, TARGET_MEM_REGION);
            }
        }
    }
    // После цикла for_each_pci_dev нужно освободить последнюю ссылку
    pci_dev_put(pdev);
    pci_dev_put(NULL);

    if (!device_found) {
        pr_warn("%s: No matching Creative AE-5 device was found in the system.", DRIVER_NAME);

        return 0;
    }

    if (!base_address) {
        pr_err("%s: No valid memory region found", DRIVER_NAME);

        return -ENOMEM;

    }

    mmio_base = ioremap(base_address, REGION_SIZE);
    if (!mmio_base) {
        pr_err("%s: Failed to remap MMIO memory", DRIVER_NAME);
        return -ENOMEM;
    }

    if (proc_create(proc_filename, 0666, NULL, &proc_file_ops) == NULL) {
        pr_err("%s: Failed to create /proc/%s", DRIVER_NAME, proc_filename);
        iounmap(mmio_base);
        return -ENOMEM;
    }

    pr_info("%s: Created /proc/%s entry", DRIVER_NAME, proc_filename);
    pr_info("%s: To set color, run: echo 0xbbggrr > /proc/%s", DRIVER_NAME, proc_filename);

    return 0;
}

static void __exit led_driver_exit(void)
{
    pr_info("%s: Unloading", DRIVER_NAME);
    remove_proc_entry(proc_filename, NULL);
    pr_info("%s: Deleted /proc/%s entry", DRIVER_NAME, proc_filename);

    if (mmio_base) {
        iounmap(mmio_base);
    }

    pr_info("%s: Unloaded", DRIVER_NAME);
}

module_init(led_driver_init);
module_exit(led_driver_exit);
