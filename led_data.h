#define LED_COUNT 5
#define PROCFS_FILENAME "sbae5_color"

// Device IDs
#define VENDOR_ID 0x1102
#define DEVICE_ID 0x0012

struct led_data {
    unsigned char led_count;
    unsigned char red_values[LED_COUNT];
    unsigned char green_values[LED_COUNT];
    unsigned char blue_values[LED_COUNT];
};