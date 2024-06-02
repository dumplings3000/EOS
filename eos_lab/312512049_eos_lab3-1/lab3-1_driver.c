#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h> //copy_to/from_user()
#include <linux/gpio.h>

// GPIO LED is connected to this GPIO
#define GPIO_LED_1 (21)
#define GPIO_LED_2 (20)
#define GPIO_LED_3 (16)
#define GPIO_LED_4 (12)

#define NUM_LEDS 4

static unsigned int gpio_leds[NUM_LEDS] = {GPIO_LED_1, GPIO_LED_2, GPIO_LED_3, GPIO_LED_4};
static dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
static int student_id = 0;

static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);

/*************** Driver functions **********************/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
/******************************************************/

// File operation structure
static struct file_operations fops =
{
    .owner   = THIS_MODULE,
    .read    = etx_read,
    .write   = etx_write,
    .open    = etx_open,
    .release = etx_release,
};

/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
    pr_info("Device File Opened...!!!\n");
    return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
    pr_info("Device File Closed...!!!\n");
    return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    char gpio_state[NUM_LEDS];
    int i;

    // Convert student ID to binary and store in gpio_state array
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_state[i] = gpio_get_value(gpio_leds[i]);
    }

    // Write the GPIO states to the user
    len = NUM_LEDS;
    if (copy_to_user(buf, gpio_state, len) > 0) {
        pr_err("ERROR: Not all the bytes have been copied to user\n");
    }
    pr_info("Read function: LED state = %d%d%d%d\n", gpio_state[3], gpio_state[2], gpio_state[1], gpio_state[0]);

    return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    uint8_t rec_buf[10] = {0};
    if (copy_from_user(rec_buf, buf, len) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }

    pr_info("Write Function: Student ID = %d\n", student_id);

    int num = atoi(rec_buf);
    int leds = 0;
    for (int i = 0; i < 4; i++) {
        if (num & (1 << i)) {
            gpio_set_value(gpio_leds[i], 1);
        }
        else
            gpio_set_value(gpio_leds[i], 0);
    }

    pr_info("Write Function : LEDs Set = %d\n", num);
    
    return len;
}

/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
    int i;

    /* Allocating Major number */
    if ((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0) {
        pr_err("Cannot allocate major number\n");
        goto r_unreg;
    }

    pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

    /* Creating cdev structure */
    cdev_init(&etx_cdev, &fops);

    /* Adding character device to the system */
    if ((cdev_add(&etx_cdev, dev, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");
        goto r_del;
    }

    /* Creating struct class */
    if ((dev_class = class_create(THIS_MODULE, "etx_class")) == NULL) {
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }

    /* Creating device */
    if ((device_create(dev_class, NULL, dev, NULL, "etx_device")) == NULL) {
        pr_err("Cannot create the Device \n");
        goto r_device;
    }

    // Requesting the GPIOs
    for (i = 0; i < NUM_LEDS; i++) {
        if (gpio_is_valid(gpio_leds[i]) == false) {
            pr_err("GPIO %d is not valid\n", gpio_leds[i]);
            goto r_device;
        }

        if (gpio_request(gpio_leds[i], "LED") < 0) {
            pr_err("ERROR: GPIO %d request\n", gpio_leds[i]);
            goto r_gpio;
        }

        gpio_direction_output(gpio_leds[i], 0);
        gpio_export(gpio_leds[i], false);
    }

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;

r_gpio:
    // Free allocated GPIOs
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_free(gpio_leds[i]);
    }
r_device:
    device_destroy(dev_class, dev);
r_class:
    class_destroy(dev_class);
r_del:
    cdev_del(&etx_cdev);
r_unreg:
    unregister_chrdev_region(dev, 1);
    return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{
    int i;

    // Unexport GPIOs and free allocated resources
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_unexport(gpio_leds[i]);
        gpio_free(gpio_leds[i]);
    }

    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);

    pr_info("Device Driver Remove...Done!!\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.0");
