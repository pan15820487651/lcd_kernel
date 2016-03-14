/* 
 * lcd_mod.c: initalizes the LCD and allows text to be printed on the screen.
 *            access is granted through the drive file.
 */

// small things to include to improve this file. might be good to grade on:
// init function will clear the contents of the data array and reset index
// init function will always clear screen after init as well

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>            // required for usage of file system
#include <linux/cdev.h>          // required for device file structs
#include <linux/semaphore.h>     // required for usage of semaphore
#include <asm/uaccess.h>
#include <linux/gpio.h>          // Required for the GPIO functions
//#include <linux/interrupt.h>     // Required for the IRQ code
#include <linux/delay.h>         // Required for msleep

// pre-processor constants used throughout the file.
#define DEVICE_NAME  "lcd"
#define BUF_LEN      24
#define DB7          70
#define DB6          71
#define DB5          72
#define DB4          73
#define DB3          74
#define DB2          75
#define DB1          76
#define DB0          78
#define E            79
#define RW           86
#define RS           62

// global variables declared static, including global variables within the file.

// variables used through function.
static struct cdev* mcdev; // ???? not sure what this is for
static dev_t dev_num;      // Holds major and minor number granted by the kernel

// defines a new structure to hold an array with a size value
typedef struct {
   unsigned int size;
   unsigned int data[];
} array;

static array allPins = {.size = 11, .data = {DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7, RW, RS, E}};

// contains data about the device.
// data : character data stored that is printed on the lcd
// sem  : semaphore value
struct fake_device {
	char data[17];
	int index;
	struct semaphore sem;
} virtual_device;

// function prototypes - usually go inside a header file.
// a good idea for future changes.
static int checkGPIOIsValid(void);
static void initializeGPIO(void);
static void initializeLCD(void);
static void setPinArray(int, int);
static void clearGPIO(void);
static void printChar(char); 
static void appendDevStr(char);

// moudle license: required to use some functionality.
MODULE_LICENSE("GPL");

/********************* FILE OPERATION FUNCTIONS ***************/

// Called on device file open
//	inode reference to file on disk, struct file represents an abstract
// checks to see if file is already open (semaphore is in use)
// prints error message if device is busy.
int device_open(struct inode *inode, struct file* filp) {
	if (down_interruptible(&virtual_device.sem) != 0) {
		printk(KERN_INFO "%s: could not lock device during open\n", DEVICE_NAME);
		return -1;
	}
	printk(KERN_INFO "%s: device opened\n", DEVICE_NAME);
	return 0;
}

// Called when user wants to get info from device file
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset) {
	printk(KERN_INFO "%s: Reading from device...\n", DEVICE_NAME);
	return copy_to_user(bufStoreData, virtual_device.data, bufCount);
}

// Called when user wants to send info to device
// will store text into string buffer and print the string to the LCD.
// LCD can only print 24 characters at a time
ssize_t device_write(struct file* filp, const char* bufSource, size_t bufCount, loff_t* curOffset) {
   //if (16 - virtual_device.index <= bufCount) return -1;
   char* writeDest = virtual_device.data + virtual_device.index;
	//int errCode = copy_from_user(writeDest, bufSource, bufCount);
	printk(KERN_ALERT "device data: %s\n writeDest: %s", virtual_device.data, writeDest);
	//virtual_device.index += bufCount;
	printk(KERN_INFO "%s: writing to device...\n", DEVICE_NAME);
	if (*bufSource == '*') { // clear display
	   setPinArray(0b0000000001, 10);
      virtual_device.data[0] = '\0';
      virtual_device.index = 0;
   }
   else {
      // prints every character of bufSource to the storage array
      // and to the LCD. Will stop adding letters once max size is reached.
      char* writeString = (char *) bufSource;
      while (*writeString && virtual_device.index < 16) {
         printk(KERN_INFO "letter printed: %c", *writeString);
         appendDevStr(*writeString);
         printChar(*writeString);
         writeString++;
      }
   }
   return 0;
}

// appends a single letter to the virtual device string
static void appendDevStr(char letter) {
   // add letter
   virtual_device.data[virtual_device.index++] = letter;
   // force next value to be string null terminator
   virtual_device.data[virtual_device.index] = '\0';
}

// prints a single character to the LCD
static void printChar(char letter) {
   int i;
   gpio_set_value(RS, 1);
   gpio_set_value(RW, 0);
   msleep(1);
   gpio_set_value(E, 1);
   msleep(1);
   // set each pin to individual bits of letter
   for (i = 0; i < 8; i++) gpio_set_value(allPins.data[i], letter >> i & 1);
   gpio_set_value(E, 0);
   msleep(1);
   gpio_set_value(RS, 0);
   gpio_set_value(RW, 1);
}

// Called upon close
// closes device and returns access to semaphore.
int device_close(struct inode* inode, struct  file *filp)
{
	up(&virtual_device.sem);
	printk(KERN_INFO "%s: closing device\n", DEVICE_NAME);
	return 0;	
}

// ensures all gpios are valid before use.
static int checkGPIOIsValid() {
   int i;
   for (i = 0; i < allPins.size; i++) {
      if (!gpio_is_valid(allPins.data[i])) return -ENODEV;
   }
   return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
   .read = device_read,
   .write = device_write,
	.open = device_open,
	.release = device_close
};

// runs on startup
// intializes module space and declares major number.
//assigns device structure data and intializes the LCD.
static int __init lcd_start(void)
{
   // assign device major dynamically from system.
   int errorCode = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
   if (errorCode < 0) {
      printk(KERN_ALERT "%s: Failed to allocate a major number\n", DEVICE_NAME);
		return errorCode;
   }
	printk(KERN_INFO "%s: major number is %d\n", DEVICE_NAME, MAJOR(dev_num));
	//printk(KERN_INFO "Use mknod /dev/%s c %d 0 for device file\n", DEVICE_NAME, major_number);
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, MAJOR(dev_num));
	
	// (2) CREATE CDEV STRUCTURE, INITIALIZING CDEV
	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;
	errorCode = cdev_add(mcdev, dev_num, 1);
	if (errorCode < 0)
	{
		printk(KERN_ALERT "%s: unable to add cdev to kernel\n", DEVICE_NAME);
		return errorCode;
	}
	// initialize semaphore and index
	sema_init(&virtual_device.sem, 1);
	virtual_device.index = 0;
	// (3) LCD initialization code.
	// check validity
	errorCode = checkGPIOIsValid();
	if (errorCode < 0) return errorCode;
	// initialize GPIOs for LCD
	initializeGPIO();
	// intialize LCD
	initializeLCD();
	printk(KERN_INFO "%s: succesfully initialized\n", DEVICE_NAME);
	return 0;
}

static void initializeGPIO() {
   int i;
   for (i = 0; i < allPins.size; i++) {
      gpio_request(allPins.data[i], "sysfs");
      gpio_direction_output(allPins.data[i], false);
      gpio_export(allPins.data[i], false);
   }
}

static void initializeLCD() {
   msleep(17);
   setPinArray(0b0000110000, 5);
   setPinArray(0b0000110000, 2);
   setPinArray(0b0000110000, 1);
   setPinArray(0b0000111000, 1);
   setPinArray(0b0000001000, 1);
   setPinArray(0b0000000001, 1);
   setPinArray(0b0000000110, 1);
   setPinArray(0b0000001111, 1);
}

// sets the individual bits of value to the contents of DB7 - DB0 and RW / RS.
// 
// value: 10 bit binary number
//
// bits:  10   9    8    7    6    5    4    3    2    1
// pins:  RS, RW, DB7, DB6, DB5, DB4, DB3, DB2, DB1, DB0
static void setPinArray(int value, int delay) {
   int i;
   for (i = 0; i < allPins.size - 1; i++)
      gpio_set_value(allPins.data[i], (value >> i) & 1);
   gpio_set_value(E, true);
   msleep(10);
   gpio_set_value(E, false);
   msleep(delay);
}

// called up exit.
// unregisters the device and all associated gpios with it.
static void __exit lcd_exit(void)
{
   clearGPIO();
	cdev_del(mcdev);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "%s: successfully unloaded\n", DEVICE_NAME);
}

// clears usage of all gpio.
// unexports and releases the pins used in the allPin array.
static void clearGPIO() {
   int i;
   for (i = 0; i < allPins.size; i++) {
      gpio_set_value(allPins.data[i], false);
      gpio_unexport(allPins.data[i]);
      gpio_free(allPins.data[i]);
   }
}

module_init(lcd_start);
module_exit(lcd_exit);

