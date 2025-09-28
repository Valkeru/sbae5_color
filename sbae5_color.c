#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/kobject.h>
#include "led_data.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Igor Kobiakov");
MODULE_DESCRIPTION("Creative SoundBlaster AE-5/AE-5 Plus LED color control");

#define REGION_SIZE 0x1024
#define LED_CONTROL_OFFSET 0x320
#define TARGET_MEM_REGION 2
#define DRIVER_NAME "SBAE5_LED_DRIVER"

// Subsystem IDs
#define SUBSYS_VENDOR_ID   0x1102
#define SUBSYS_AE5_DEVICE_ID      0x0051
#define SUBSYS_AE5_PLUS_DEVICE_ID 0x0191

// Mapped memory area pointer
static void __iomem *mmio_base;

static bool proc_file_created = false;
static bool kobject_created = false;

// Base address to map
unsigned long long base_address = 0;

// Detected device name to be sent to user space
static const char* device_name;
// Detected device location to be sent to user space
static const char* device_location;

// Kernel object for interaction using sysfs
static struct kobject *sbae_kobject;

/**
 * Provides device location on PCI bus
 * Calls by kernel when user reads /sys/kernel/sbae5_color/device_location
 *
 * @param kobj Kernel object
 * @param attr Attribute to read
 * @param buf Buffer to send to user
 * @return Total bytes written
 */
static ssize_t device_location_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "PCI: %s\n", device_location);
}

/**
 * Provides device name
 * Calls by kernel when user reads /sys/kernel/sbae5_color/device_name
 *
 * @param kobj Kernel object
 * @param attr Attribute to read
 * @param buf Buffer to send to user
 * @return Total bytes written
 */
static ssize_t device_name_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", device_name);
}

/**
 * Register attributes name and location
 */
static struct kobj_attribute device_location_attribute = __ATTR(device_location, 0664, device_location_show, NULL);
static struct kobj_attribute device_name_attribute = __ATTR(device_name, 0664, device_name_show, NULL);

static void write_bit(bool bit) {
    if (!mmio_base) {
        pr_err("%s: MMIO base not mapped\n", DRIVER_NAME);
        return;
    }

    if (bit) {
        // Set the data bit if bit is 1
        iowrite32(0x102, mmio_base + LED_CONTROL_OFFSET);
        // Clock the data bit
        iowrite32(0x103, mmio_base + LED_CONTROL_OFFSET);
        // Toggle the clock
        iowrite32(0x03, mmio_base + LED_CONTROL_OFFSET);
    } else {
        // Send bit
        iowrite32(0x02, mmio_base + LED_CONTROL_OFFSET);
        // Clock the data bit
        iowrite32(0x103, mmio_base + LED_CONTROL_OFFSET);
        // Toggle the clock
        iowrite32(0x03, mmio_base + LED_CONTROL_OFFSET);
    }
}

static void set_led_color(unsigned char red_values, unsigned char green_values, unsigned char blue_values) {
    // Send brightness bits
    for (int i = 0; i < 8; i++) {
        write_bit(true);
    }

    // Get hex color value
    uint32_t led_color = blue_values << 16 | green_values << 8 | red_values;
    for (int i = 0; i < 24; i++) {
        // Current bit
        uint32_t bit = (led_color >> (23 - i)) & 0x01;
        write_bit(bit == 1);
    }
}

/**
 * LED control color
 * @param led_color Led color code to set
 */
static void handle_colors(struct led_data data)
{
    // Send start frame - 32 0's
    for (int i = 0; i < 32; i++) {
        write_bit(false);
    }

    unsigned char led_count = data.led_count;
    for (int i = 0; i < led_count; i++) {
        set_led_color(data.red_values[i], data.green_values[i], data.blue_values[i]);
    }

    // Send end frame - 32 1's
    for (int i = 0; i < 32; i++) {
        write_bit(true);
    }
}

/**
 * Called by kernel on write into /proc/sbae5_led
 *
 * @param file File
 * @param __user Input data buffer
 * @return Total bytes count
 */
static ssize_t on_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
    struct led_data data;

    if (count > sizeof(data)) {
        pr_err("%s: Input too long\n", DRIVER_NAME);

        return -EFAULT;
    }

    if (copy_from_user(&data, buffer, count)) {
        return -EFAULT;
    }

    handle_colors(data);

    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_write = on_proc_write,
};

static int __init led_driver_init(void)
{
    struct pci_dev *pdev = NULL;
    bool device_found = false;
    resource_size_t base_address;

    pr_info("%s: Searching for Creative AE-5 devices\n", DRIVER_NAME);

    for_each_pci_dev(pdev) {
        if (pdev->vendor == VENDOR_ID && pdev->device == DEVICE_ID) {
            const char *dev_name;
            device_found = true;

            // Check device ID to detect particular model
            if (pdev->subsystem_vendor == SUBSYS_VENDOR_ID && pdev->subsystem_device == SUBSYS_AE5_PLUS_DEVICE_ID) {
                dev_name = "Creative SoundBlaster AE-5 Plus";
            } else if (pdev->subsystem_vendor == SUBSYS_VENDOR_ID && pdev->subsystem_device == SUBSYS_AE5_DEVICE_ID) {
                dev_name = "Creative SoundBlaster AE-5";
            } else {
                dev_name = "Creative SoundBlaster AE-5 (Unknown Subsystem)";
            }

            const char* location = pci_name(pdev);
            pr_info("%s: Found device: %s at PCI location %s\n", DRIVER_NAME, dev_name, pci_name(pdev));

            // Get region 2 starting address
            base_address = pci_resource_start(pdev, TARGET_MEM_REGION);

            if (base_address) {
                pr_info("%s: Physical Base Address of BAR %d is: 0x%pa\n",
                        DRIVER_NAME, TARGET_MEM_REGION, &base_address);

                device_name = dev_name;
                device_location = location;
                break;
            } else {
                pr_warn("%s: Device found, but BAR %d is not available or has a zero address\n", DRIVER_NAME, TARGET_MEM_REGION);
            }
        }
    }

    pci_dev_put(pdev);

    if (!device_found) {
        pr_warn("%s: No matching Creative AE-5 device was found in the system\n", DRIVER_NAME);

        return 0;
    }

    if (!base_address) {
        pr_err("%s: No valid memory region found\n", DRIVER_NAME);

        return -ENOMEM;

    }

    mmio_base = ioremap(base_address, REGION_SIZE);
    if (!mmio_base) {
        pr_err("%s: Failed to remap MMIO memory\n", DRIVER_NAME);
        return -ENOMEM;
    }

    if (proc_create(PROCFS_FILENAME, 0666, NULL, &proc_file_ops) == NULL) {
        pr_err("%s: Failed to create /proc/%s\n", DRIVER_NAME, PROCFS_FILENAME);
        iounmap(mmio_base);
        return -ENOMEM;
    }
    proc_file_created = true;

    pr_info("%s: Created /proc/%s entry", DRIVER_NAME, PROCFS_FILENAME);

    sbae_kobject = kobject_create_and_add("sbae5_color", kernel_kobj);
    if (!sbae_kobject) {
        pr_err("%s: Failed to create kernel object\n", DRIVER_NAME);

        return -ENOMEM;
    }

    int error = sysfs_create_file(sbae_kobject, &device_name_attribute.attr);
    if (error) {
        pr_err("%s: Failed to create the device_name file in /sys/kernel/sbae5_color\n", DRIVER_NAME);

        return -ENOMEM;
    }

    error = sysfs_create_file(sbae_kobject, &device_location_attribute.attr);
    if (error) {
        pr_err("%s: Failed to create the device_location file in /sys/kernel/sbae5_color\n", DRIVER_NAME);

        return -ENOMEM;
    }
    kobject_created = true;

    return 0;
}

static void __exit led_driver_exit(void)
{
    pr_info("%s: Unloading\n", DRIVER_NAME);
    if  (proc_file_created) {
        remove_proc_entry(PROCFS_FILENAME, NULL);
        pr_info("%s: Deleted /proc/%s entry\n", DRIVER_NAME, PROCFS_FILENAME);
    }

    if (mmio_base) {
        iounmap(mmio_base);
    }

    if (kobject_created) {
        kobject_put(sbae_kobject);
    }

    pr_info("%s: Unloaded\n", DRIVER_NAME);
}

module_init(led_driver_init);
module_exit(led_driver_exit);
