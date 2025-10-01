#include <linux/pci.h>
#include "sbae5_color.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Igor Kobiakov");
MODULE_DESCRIPTION("Creative SoundBlaster AE-5/AE-5 Plus LED driver");

// Device operations
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .unlocked_ioctl = ioctl_handler
};

// Device data with fields populated with empty strings
static struct device_data device_data = {
        .name = "",
        .location = ""
};

// Mapped memory area pointer
static void __iomem *mmio_base;

// Device info
static dev_t dev_number;
static struct class *dev_class;
static struct cdev chardev;

module_init(led_driver_init);
module_exit(led_driver_exit);

/**
 * Initialize device
 * Scans PCI devices and sets name and location for device_data
 */
static int __init led_driver_init()
{
    struct pci_dev *pdev = NULL;
    bool device_found = false;
    unsigned long long base_address;

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
            pr_info("%s: Found device: %s at PCI location %s\n", DRIVER_NAME, dev_name, location);

            // Get region 2 starting address
            base_address = pci_resource_start(pdev, TARGET_MEM_REGION);

            if (base_address) {
                pr_info("%s: Physical Base Address of BAR %d is: 0x%pa\n",
                        DRIVER_NAME, TARGET_MEM_REGION, &base_address);

                strncpy(device_data.name, dev_name, NAME_MAX_LEN - 1);
                strncpy(device_data.location, location, LOCATION_MAX_LEN - 1);
                device_data.name[NAME_MAX_LEN - 1] = '\0';
                device_data.location[LOCATION_MAX_LEN - 1] = '\0';
                break;
            } else {
                pr_warn("%s: Device found, but BAR %d is not available or has a zero address\n", DRIVER_NAME, TARGET_MEM_REGION);
            }
        }
    }

    pci_dev_put(pdev);

    if (!device_found) {
        pr_info("%s: No matching Creative AE-5 device was found in the system\n", DRIVER_NAME);

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

    int device_check = create_device();
    if (device_check < 0) {
        pr_err("%s: Failed to create device", DRIVER_NAME);

        return device_check;
    }

    pr_info("%s: Driver successfully loaded", DRIVER_NAME);

    return 0;
}

int create_device() {
    if (alloc_chrdev_region(&dev_number, 0, 1, DEV_FILENAME) < 0) {
        pr_err("%s: Failed to create /dev/%s\n", DRIVER_NAME, DEV_FILENAME);
        iounmap(mmio_base);
        return -ENOMEM;
    }

    dev_class = class_create("argb-control");
    if (IS_ERR(dev_class)) {
        unregister_chrdev_region(dev_number, 1);

        return -ENOMEM;
    }

    // Don't forget to chmod 666 or use udev rules
    if (device_create(dev_class, NULL, dev_number, NULL, DEV_FILENAME) == NULL) {
        pr_err("failed to create device\n");

        class_destroy(dev_class);
        unregister_chrdev_region(dev_number, 1);
    }

    pr_info("%s: Created /dev/%s entry", DRIVER_NAME, DEV_FILENAME);

    cdev_init(&chardev, &fops);
    chardev.owner = THIS_MODULE;
    if (cdev_add(&chardev, dev_number, 1) < 0) {
        pr_err("%s: Failed to create chardev\n", DRIVER_NAME);
        if (mmio_base) {
            iounmap(mmio_base);
        }
        device_destroy(dev_class, dev_number);
        class_destroy(dev_class);
        class_destroy(dev_class);

        return -ENOMEM;
    }

    return 0;
}

static long ioctl_handler(struct file *file, unsigned int command, unsigned long argument) {
    pr_info("%s: ioctl handler called\n", DRIVER_NAME);

    switch (command) {
        case IOCTL_COMMAND_READ_DEVICE_INFO:
            pr_info("%s: Sending device info\n", DRIVER_NAME);

            if (copy_to_user((void __user*) argument, &device_data, sizeof(device_data))) {
                pr_err("%s: Failed to send device info\n", DRIVER_NAME);
            }

            pr_info("%s: Device info successfully sent\n", DRIVER_NAME);
            break;
        case IOCTL_COMMAND_SET_INTERNAL_COLOR:
            pr_info("%s: Setting up LEDs\n", DRIVER_NAME);

            struct led_data received_data;
            if (copy_from_user(&received_data, (void __user *) argument, sizeof(received_data))) {
                return -ENOMEM;
            }

            if (received_data.led_count > LED_COUNT) {
                pr_err("%s: Invalid LED count received: %d. Max LED count is %d", DRIVER_NAME, received_data.led_count, LED_COUNT);

                return -ENOMEM;
            }

            handle_colors(received_data);
            break;
        default:
            pr_err("%s: Unknown command!\n", DRIVER_NAME);
            return -ENOMEM;
    }

    return 0;
}

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

static void __exit led_driver_exit()
{
    pr_info("%s: Unloading\n", DRIVER_NAME);

    cdev_del(&chardev);
    device_destroy(dev_class, dev_number);
    class_destroy(dev_class);
    class_destroy(dev_class);

    if (mmio_base) {
        iounmap(mmio_base);
    }

    pr_info("%s: Unloaded\n", DRIVER_NAME);
}