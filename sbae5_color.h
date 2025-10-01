
#ifndef SBAE5_COLOR_SBAE5_COLOR_H
#define SBAE5_COLOR_SBAE5_COLOR_H

#include <linux/cdev.h>
#include "led_data.h"

#define DRIVER_NAME        "SBAE5_LED_DRIVER"
#define REGION_SIZE        0x1024
#define TARGET_MEM_REGION  2
#define LED_CONTROL_OFFSET 0x320

// Subsystem IDs
#define SUBSYS_VENDOR_ID          0x1102
#define SUBSYS_AE5_DEVICE_ID      0x0051
#define SUBSYS_AE5_PLUS_DEVICE_ID 0x0191

/*
 * Init handler
 * Scans PCI devices to find devices
 */
static int __init led_driver_init(void);

/*
 * Exit handler
 * Remove devices, release resources
 */
static void __exit led_driver_exit(void);

/*
 * Creates device file in /dev filesystem
 */
static int create_device(void);

/*
 * ioctl syscalls handler
 */
static long ioctl_handler(struct file* file, unsigned int command, unsigned long argument);

/*
 * LED color control
 */
static void handle_colors(struct led_data data);

static void write_bit(bool bit);
static void set_led_color(unsigned char red_values, unsigned char green_values, unsigned char blue_values);

#endif //SBAE5_COLOR_SBAE5_COLOR_H
