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
#define GPIO_LED_1 (8)
#define GPIO_LED_2 (7)
#define GPIO_LED_3 (6)
#define GPIO_LED_4 (5)
#define GPIO_LED_5 (4)
#define GPIO_LED_6 (3)
#define GPIO_LED_7 (2)
#define GPIO_LED_8 (1)
#define NUM_LEDS 8
// GPIO pins for 7-segment display
#define SEG_A (11)
#define SEG_B (12)
#define SEG_C (13)
#define SEG_D (14)
#define SEG_E (15)
#define SEG_F (16)
#define SEG_G (17)
#define NUM_SEG 7

const int segment[10][7] = {
        {1, 1, 1, 1, 1, 1, 0}, // 0
        {0, 1, 1, 0, 0, 0, 0}, // 1
        {1, 1, 0, 1, 1, 0, 1}, // 2
        {1, 1, 1, 1, 0, 0, 1}, // 3
        {0, 1, 1, 0, 0, 1, 1}, // 4
        {1, 0, 1, 1, 0, 1, 1}, // 5
        {1, 0, 1, 1, 1, 1, 1}, // 6
        {1, 1, 1, 0, 0, 0, 0}, // 7
        {1, 1, 1, 1, 1, 1, 1}, // 8
        {1, 1, 1, 1, 0, 1, 1}  // 9
    };


static unsigned int gpio_leds[] = {GPIO_LED_1, GPIO_LED_2, GPIO_LED_3, GPIO_LED_4, GPIO_LED_5,GPIO_LED_6,GPIO_LED_7,GPIO_LED_8};
static unsigned int seg_pins[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};

static dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

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
    pr_info("Read function:" );

    return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    uint8_t distance_buf[9] = {0};
    uint8_t price_buf[9] = {0};
    int distance;
    int digit_distance;
    int price;
    int digit_price;
    int res;
    int n = 0;
    int mul;
    int num_price = 0;
    
    if (copy_from_user(distance_buf, buf, 1) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }

    if (copy_from_user(price_buf, buf+1, len-1) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }
    
    res = kstrtoint(price_buf, 10, &price);
    res = kstrtoint(distance_buf, 10, &distance);

    pr_err("distance %d \n", distance);
    pr_err("price %d \n", price);
    digit_distance = distance;
    digit_price = price;

    while(digit_price != 0) {
        digit_price /= 10;
        num_price++;
    }

    while(num_price >= 0 || digit_distance >= 0 ){
        pr_err("n %d \n", n);
        digit_price = price;
        // 7 Segment control
        mul = 1;
        while((digit_price) > 9){
            digit_price /= 10;
            mul *=10;
        };
        pr_err("digit_price:  %d \n", digit_price);
        for(int i = 0; i < NUM_SEG ; i++){
            gpio_set_value(seg_pins[i], segment[digit_price][i]);
        }
        if(num_price < 0){
            for(int i = 0; i < NUM_SEG ; i++){
                gpio_set_value(seg_pins[i], 0);
            }
        }
        price %= mul ;
        num_price--;
        // led control
        if(n % 2 ==0 && digit_distance >= 0){
            pr_err("digit_distance:  %d \n", digit_distance);
            for (int i = 0; i < digit_distance; i++) {
                gpio_set_value(gpio_leds[i], 1);
            }
            for (int i = 0; i < (distance - digit_distance); i++) {
                gpio_set_value(gpio_leds[digit_distance], 0);
            }
            digit_distance--;
            n++;
        }
        else{
            n++;
        }
        msleep(500);
    }

    
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

    // Requesting the led GPIOs
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

    // Requesting the seg GPIOs
    for (i = 0; i < NUM_SEG; i++) {
        if (gpio_is_valid(seg_pins[i]) == false) {
            pr_err("GPIO %d is not valid\n", seg_pins[i]);
            goto r_device;
        }

        if (gpio_request(seg_pins[i], "SEG") < 0) {
            pr_err("ERROR: GPIO %d request\n", seg_pins[i]);
            goto r_gpio;
        }

        gpio_direction_output(seg_pins[i], 0);
        gpio_export(seg_pins[i], false);
    }

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;
    

r_gpio:
    // Free allocated GPIOs
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_free(gpio_leds[i]);
    }
    for (i = 0; i < NUM_SEG; i++) {
        gpio_free(seg_pins[i]);
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