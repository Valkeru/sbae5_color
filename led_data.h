#include <linux/ioctl.h>

#define INTERNAL_LED_COUNT 5
#define DEV_FILENAME "sbae5-color"

// Device IDs
#define VENDOR_ID 0x1102
#define DEVICE_ID 0x0012

#define NAME_MAX_LEN 100
#define LOCATION_MAX_LEN 100

#define IOCTL_MAGIC 'l'
#define IOCTL_COMMAND_READ_DEVICE_INFO _IOR(IOCTL_MAGIC, 0, struct device_data)
#define IOCTL_COMMAND_SET_INTERNAL_COLOR _IOW(IOCTL_MAGIC, 1, struct led_data)

struct led_data {
    unsigned char led_count;
    unsigned char red_values[INTERNAL_LED_COUNT];
    unsigned char green_values[INTERNAL_LED_COUNT];
    unsigned char blue_values[INTERNAL_LED_COUNT];
};

struct device_data {
    char name[NAME_MAX_LEN];
    char location[LOCATION_MAX_LEN];
};
