#include <linux/ioctl.h>

#define LED_COUNT 5
#define PROCFS_FILENAME "sbae5_color"

// Device IDs
#define VENDOR_ID 0x1102
#define DEVICE_ID 0x0012

#define NAME_MAX_LEN 100
#define LOCATION_MAX_LEN 100

#define IOCTL_COMMAND_READ_DEVICE_INFO _IOR('l', 0, struct device_data)
#define IOCTL_COMMAND_SET_INTERNAL_COLOR _IOW('l', 1, struct led_data)

struct led_data {
    unsigned char led_count;
    unsigned char red_values[LED_COUNT];
    unsigned char green_values[LED_COUNT];
    unsigned char blue_values[LED_COUNT];
};

struct device_data {
    char name[NAME_MAX_LEN];
    char location[LOCATION_MAX_LEN];
};
