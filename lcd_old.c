#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>                 // Required for the GPIO functions
#include <linux/interrupt.h>            // Required for the IRQ code
#include <linux/delay.h>

MODULE_LICENSE("GPL");

struct fake_device {
	char data[100];
	struct semaphore sem;
} virtual_device;

struct cdev* mcdev;
int major_number;
int ret; // returns from a function

//static unsigned int DB7 = 70;
//static unsigned int DB6 = 71;
//static unsigned int DB5 = 72;
//static unsigned int DB4 = 73;
//static unsigned int DB3 = 74;
//static unsigned int DB2 = 75;
//static unsigned int DB1 = 76;
//static unsigned int DB0 = 78;
static unsigned int E = 79;
static unsigned int RW = 86;
static unsigned int RS = 62;

static unsigned int allPins[] = {70, 71, 72, 73, 74, 75, 76, 78, 79, 86, 62};
static unsigned int initArray[] = {78, 76, 75, 74, 73, 72, 71, 70, 86, 62};

static int checkGPIOIsValid(void);
static void initializeGPIO(void);
static void initializeLCD(void);
static void setPinArray(int, int);
static void clearGPIO(void);
static void printChar(char); 
dev_t dev_num; // Hold major number that kernel gives us
#define DEVICE_NAME "lcd_old"

/********************* FILE OPERATION FUNCTIONS ***************/
// Called on device file open
//	inode reference to file on disk, struct file represents an abstract
//	open file
int device_open(struct inode *inode, struct file* filp)
{
	if (down_interruptible(&virtual_device.sem) != 0)
	{
		printk(KERN_ALERT "new_char: could not lock device during open\n");
		return -1;
	}
	printk(KERN_INFO "new_char: device opened\n");
	return 0;
}

// Called when user wants to get info from device file
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset)
{
	printk(KERN_INFO "new_char: Reading from device...\n");
	// copy_to_user (destination, source, sizeToTransfer)
	ret = copy_to_user(bufStoreData, virtual_device.data, bufCount);
	return ret;
}

// Called when user wants to send info to device
ssize_t device_write(struct file* filp, const char* bufSource, size_t bufCount, loff_t* curOffset)
{
	printk(KERN_INFO "new_char: writing to device...\n");
	ret = copy_from_user(virtual_device.data, bufSource, bufCount);
	if (*bufSource == 'c') setPinArray(0b0000000001, 10);
   // print character
   else printChar(*bufSource);
	return ret;
}

static void printChar(char letter) {
   int i;
   gpio_set_value(RS, 1);
   gpio_set_value(RW, 0);
   msleep(1);
   gpio_set_value(E, 1);
   msleep(1);
   for (i = 0; i < 8; i++) gpio_set_value(initArray[i], letter >> i & 1);
   gpio_set_value(E, 0);
   msleep(1);
   gpio_set_value(RS, 0);
   gpio_set_value(RW, 1);
}

// Called upon close
int device_close(struct inode* inode, struct  file *filp)
{
	up(&virtual_device.sem);
	printk(KERN_INFO "new_char, closing device\n");
	return 0;	
}

static int checkGPIOIsValid() {
   int i;
   for (i = 0; i < 11; i++) {
      if (!gpio_is_valid(allPins[i])) return -ENODEV;
   }
   return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,
	.write = device_write,
	.read = device_read
};

static int __init lcd_start(void)
{
   int result = 0;
   // assign device major dynamically from system.
   ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
   if (ret < 0) {
      printk(KERN_ALERT "LCD_module: Failed to allocate a major number\n");
		return ret;
   }
	major_number = MAJOR(dev_num);
	printk(KERN_INFO "LCD_module: major number is %d\n", major_number);
	//printk(KERN_INFO "Use mknod /dev/%s c %d 0 for device file\n", DEVICE_NAME, major_number);
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, major_number);
	// (2) CREATE CDEV STRUCTURE, INITIALIZING CDEV
	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;
	ret = cdev_add(mcdev, dev_num, 1);
	if (ret < 0)
	{
		printk(KERN_ALERT "LCD_module: unable to add cdev to kernel\n");
		return ret;
	}
	// initialize semaphore.
	sema_init(&virtual_device.sem, 1);
	
	// (3) LCD initialization code.
	// check validity
	result = checkGPIOIsValid();
	if (result < 0) return result;
	// initialize GPIOs for LCD
	initializeGPIO();
	// intialize LCD
	initializeLCD();
	printk("LCD_module: succesfully initialized\n");
	return 0;
}

static void initializeGPIO() {
   int i;
   for (i = 0; i < 11; i++) {
      gpio_request(allPins[i], "sysfs");
      gpio_direction_output(allPins[i], false);
      gpio_export(allPins[i], false);
   }
}

static void initializeLCD() {
   msleep(17);
   printk("Inside init function LCD for register flipping\n");
   setPinArray(0b0000110000, 5);
   setPinArray(0b0000110000, 2);
   setPinArray(0b0000110000, 1);
   setPinArray(0b0000111000, 1);
   setPinArray(0b0000001000, 1);
   setPinArray(0b0000000001, 1);
   setPinArray(0b0000000110, 1);
   setPinArray(0b0000001111, 1);
}

static void setPinArray(int value, int delay) {
   int i;
   for (i = 0; i < 10; i++) gpio_set_value(initArray[i], (value >> i) & 1);
   gpio_set_value(E, true);
   msleep(10);
   gpio_set_value(E, false);
   msleep(delay);
}

static void __exit lcd_exit(void)
{
   clearGPIO();
	cdev_del(mcdev);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "LCD_module: successfully unloaded\n");
}

static void clearGPIO() {
   int i;
   for (i = 0; i < 11; i++) {
      gpio_set_value(allPins[i], false);
      gpio_unexport(allPins[i]);
      gpio_free(allPins[i]);
   }
}

module_init(lcd_start);
module_exit(lcd_exit);

