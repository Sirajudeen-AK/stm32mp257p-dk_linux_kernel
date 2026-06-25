#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>


/*
*		Interviewers often ask:
*		Why not just use register_chrdev()?
*		
*		Answer:
*		
*		Problems of register_chrdev():
*		- Old API
*		- Manages only major number easily
*		- Less flexible
*		- Internally creates a large range of minors
*
*		Modern drivers use alloc_chrdev_region(), cdev_init(), and cdev_add() because they provide explicit control over 
*		major/minor allocation and support multiple devices cleanly.
*
*		No need to use makenode() to create device nodes in /dev. The device_create() function automatically creates the 
*		device node when the driver is loaded, simplifying the process and ensuring proper permissions.
*			
*		
*/

#define DEVICE_NAME "mycdev_new"

/* dev_t - Major + Minor*/
static dev_t dev_num;

/* Character device instance */
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

/* file operations */
static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mycdev: open called\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mycdev: release called\n");
    return 0;
}

#define BUF_SIZE 100
static char kernel_buffer[BUF_SIZE];

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    if (len > BUF_SIZE)
        len = BUF_SIZE;

    if (copy_from_user(kernel_buffer, buf, len))
        return -EFAULT;

    pr_info("Received: %s\n", kernel_buffer);

    return len;
}

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    int data_len = strlen(kernel_buffer);

    if (*off >= data_len)
        return 0;

    if (len > data_len)
        len = data_len;

    if (copy_to_user(buf, kernel_buffer, len))
        return -EFAULT;

	/*
		Why update *off in read?
			To support EOF and partial reads
	*/
    *off += len;

    return len;
}


static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int num_devices = 1;

static int __init my_init(void)
{
	
	/*
	Arguments:
		&dev_num  -> kernel fills major/minor
		0         -> first minor
		1         -> number of minors
		"mychar"  -> name
	*/
    if (alloc_chrdev_region(&dev_num, 0, num_devices, DEVICE_NAME) < 0)
        return -1;

	/* connects with file operations - when device is opened, use fops callbacks */
    cdev_init(&my_cdev, &fops);

	/* Registers device with kernel.*/
    if (cdev_add(&my_cdev, dev_num, num_devices) < 0)
        goto unregister;

	/* Creates a device class under: /sys/class/ */
    my_class = class_create(DEVICE_NAME);
    if (IS_ERR(my_class))
        goto del_cdev;

	/* Creates a device node under: /dev/

	Arguments:
		my_class   -> class pointer
		NULL       -> parent device
		dev_num    -> dev_t (major + minor)
		NULL       -> device data
		DEVICE_NAME-> name of device node in /dev

	What creates /dev/mychar?
		Linux userspace daemon creates the node.
			device_create()
				|
				V
			sysfs event (uevent)
				|
				V
			udev/mdev
				|
				V
			/dev/mychar

	*/
    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device))
        goto destroy_class;

	/* Kernel provides macros: MAJOR & MINOR */
    pr_info("mycdev loaded major=%d minor=%d\n",
            MAJOR(dev_num), MINOR(dev_num));

    return 0;

destroy_class:
    class_destroy(my_class);
del_cdev:
    cdev_del(&my_cdev);
unregister:
    unregister_chrdev_region(dev_num, num_devices);
    return -1;
}

/* exit */
static void __exit my_exit(void)
{
	/*
	Why reverse order?
		Reason: Destroying lower layer first leaves dangling references.
	*/
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, num_devices);

    pr_info("mycdev unloaded\n");
}
/* 
module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("STM32 cdev driver"); */