#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  //copy_to/from_user()
#include <linux/gpio.h>  //GPIO
#include <linux/timer.h>

//LED is connected to this GPIO
// 1 : virtical red
// 2 : virtical green
// 3 : parallel red
// 4 : parallel green
// 5 : pedestrian
int LED_GPIO[] = {1, 2, 3, 4, 5};
int SEVEN_GPIO[] = {11, 12, 13, 14, 15, 16, 17};
int number[][7] = {
	{1, 1, 1, 1, 1, 1, 0},  
	{0, 1, 1, 0, 0, 0, 0},
	{1, 1, 0, 1, 1, 0, 1},
	{1, 1, 1, 1, 0, 0, 1},
	{0, 1, 1, 0, 0, 1, 1},
	{1, 0, 1, 1, 0, 1, 1},
	{1, 0, 1, 1, 1, 1, 1},
	{1, 1, 1, 0, 0, 0, 0},
	{1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 0, 1, 1}};

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);

/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
/******************************************************/

//File operation structure
static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.read = etx_read,
	.write = etx_write,
	.open = etx_open,
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
    pr_info("Read\n");
	return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{	
  	int rec_buf[256] = {0};
  	if(copy_from_user(rec_buf, buf, len ) > 0) {
		pr_err("ERROR: Not all the bytes have been copied from user\n");
	}
	
	
	// pedestrian
	if (rec_buf[0] == '0'){   
		gpio_set_value(LED_GPIO[0], 1);
		gpio_set_value(LED_GPIO[1], 0);
		gpio_set_value(LED_GPIO[2], 1);
		gpio_set_value(LED_GPIO[3], 0);
		gpio_set_value(LED_GPIO[4], 1);
	// virtical green light
	} else if (rec_buf[0] == '1'){   
		gpio_set_value(LED_GPIO[0], 0);
		gpio_set_value(LED_GPIO[1], 1);
		gpio_set_value(LED_GPIO[2], 1);
		gpio_set_value(LED_GPIO[3], 0);
		gpio_set_value(LED_GPIO[4], 0);
	// parallel green light
	} else if (rec_buf[0] == '2'){   
		gpio_set_value(LED_GPIO[0], 1);
		gpio_set_value(LED_GPIO[1], 0);
		gpio_set_value(LED_GPIO[2], 0);
		gpio_set_value(LED_GPIO[3], 1);
		gpio_set_value(LED_GPIO[4], 0);
	}
	
	// seven segment displayer
	for (int a = 0; a < 7; a++){
			gpio_set_value(SEVEN_GPIO[a], number[rec_buf[2]][a]);
	}	
	return len;
}

/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
	/*Allocating Major number*/
	if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0){
		pr_err("Cannot allocate major number\n");
		goto r_unreg;
	}
	pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
	/*Creating cdev structure*/
	cdev_init(&etx_cdev,&fops);
	/*Adding character device to the system*/
	if((cdev_add(&etx_cdev,dev,1)) < 0){
		pr_err("Cannot add the device to the system\n");
		goto r_del;
	}
	/*Creating struct class*/
	if((dev_class = class_create(THIS_MODULE,"etx_class")) == NULL){
		pr_err("Cannot create the struct class\n");
		goto r_class;
	}
	/*Creating device*/
	if((device_create(dev_class,NULL,dev,NULL,"etx_device")) == NULL){
		pr_err( "Cannot create the Device \n");
		goto r_device;
	}
	
	//Checking the GPIO is valid or not
	for (int i = 0; i < 6; ++i) {
        if(gpio_is_valid(LED_GPIO[i]) == false){
			pr_err("GPIO %d is not valid\n", LED_GPIO[i]);
			goto r_device;
		}
    }
    for (int i = 0; i < 7; ++i) {
        if(gpio_is_valid(SEVEN_GPIO[i]) == false){
			pr_err("GPIO %d is not valid\n", SEVEN_GPIO[i]);
			goto r_device;
		}
    }

	//Requesting the GPIO
	if(gpio_request(1,"GPIO_1") < 0){
		pr_err("ERROR: GPIO %d request\n", 1);
		goto r_gpio;
	}
	if(gpio_request(2,"GPIO_2") < 0){
		pr_err("ERROR: GPIO %d request\n", 2);
		goto r_gpio;
	}
	if(gpio_request(3,"GPIO_3") < 0){
		pr_err("ERROR: GPIO %d request\n", 3);
		goto r_gpio;
	}
	if(gpio_request(4,"GPIO_4") < 0){
		pr_err("ERROR: GPIO %d request\n", 4);
		goto r_gpio;
	}
	if(gpio_request(5,"GPIO_5") < 0){
		pr_err("ERROR: GPIO %d request\n", 5);
		goto r_gpio;
	}
	
	if(gpio_request(11,"GPIO_11") < 0){
		pr_err("ERROR: GPIO %d request\n", 11);
		goto r_gpio;
	}
	if(gpio_request(12,"GPIO_12") < 0){
		pr_err("ERROR: GPIO %d request\n", 12);
		goto r_gpio;
	}
	if(gpio_request(13,"GPIO_13") < 0){
		pr_err("ERROR: GPIO %d request\n", 13);
		goto r_gpio;
	}
	if(gpio_request(14,"GPIO_14") < 0){
		pr_err("ERROR: GPIO %d request\n", 14);
		goto r_gpio;
	}
	if(gpio_request(15,"GPIO_15") < 0){
		pr_err("ERROR: GPIO %d request\n", 15);
		goto r_gpio;
	}
	if(gpio_request(16,"GPIO_16") < 0){
		pr_err("ERROR: GPIO %d request\n", 16);
		goto r_gpio;
	}
	if(gpio_request(17,"GPIO_17") < 0){
		pr_err("ERROR: GPIO %d request\n", 17);
		goto r_gpio;
	}

	//configure the GPIO as output
	for (int i = 0; i < 5; ++i) {
        gpio_direction_output(LED_GPIO[i], 0);
    }
    for (int i = 0; i < 7; ++i) {
        gpio_direction_output(SEVEN_GPIO[i], 0);
    }
	
	/* Using this call the GPIO 21 will be visible in /sys/class/gpio/
	** Now you can change the gpio values by using below commands also.
	** echo 1 > /sys/class/gpio/gpio21/value (turn ON the LED)
	** echo 0 > /sys/class/gpio/gpio21/value (turn OFF the LED)
	** cat /sys/class/gpio/gpio21/value (read the value LED)
	**
	** the second argument prevents the direction from being changed.
	*/
	for (int i = 0; i < 5; ++i) {
        gpio_export(LED_GPIO[i], false);
    }
    for (int i = 0; i < 7; ++i) {
        gpio_export(SEVEN_GPIO[i], false);
    }

	pr_info("Device Driver Insert...Done!!!\n");
	return 0;

r_gpio:
	for (int i = 0; i < 5; ++i) {
        gpio_free(LED_GPIO[i]);
    }
    for (int i = 0; i < 7; ++i) {
        gpio_free(SEVEN_GPIO[i]);
    }
r_device:
	device_destroy(dev_class,dev);
r_class:
	class_destroy(dev_class);
r_del:
	cdev_del(&etx_cdev);
r_unreg:
	unregister_chrdev_region(dev,1);
	return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{
	
	for (int i = 0; i < 5; ++i) {
        gpio_unexport(LED_GPIO[i]);
        gpio_free(LED_GPIO[i]);
    }
    for (int i = 0; i < 7; ++i) {
        gpio_unexport(SEVEN_GPIO[i]);
        gpio_free(SEVEN_GPIO[i]);
    }
	device_destroy(dev_class,dev);
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
MODULE_VERSION("1.32");