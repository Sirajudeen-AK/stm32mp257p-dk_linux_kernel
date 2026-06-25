#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

/* Multiple Minor devices */
#define NUM_DEVICES 4

#define BUF_SIZE    100

static dev_t dev_num;
static struct cdev my_cdev;

static struct class *my_class;
static struct device *my_device[NUM_DEVICES];

static char kernel_buffer[NUM_DEVICES][BUF_SIZE];

static int my_open(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);

    pr_info("mychar%d opened\n", minor);

    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);

    pr_info("mychar%d closed\n", minor);

    return 0;
}

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    int minor = iminor(file_inode(file));

    if (len >= BUF_SIZE)
        len = BUF_SIZE - 1;

    if (copy_from_user(kernel_buffer[minor], buf, len))
        return -EFAULT;

    kernel_buffer[minor][len] = '\0';

    pr_info("mychar%d received: %s\n",
            minor,
            kernel_buffer[minor]);

    return len;
}

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    int minor = iminor(file_inode(file));
    int data_len = strlen(kernel_buffer[minor]);

    if (*off >= data_len)
        return 0;

    if (len > data_len - *off)
        len = data_len - *off;

    if (copy_to_user(buf,
                     kernel_buffer[minor] + *off,
                     len))
        return -EFAULT;

    *off += len;

    return len;
}

static struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

/* sysfs attribute show function */
static ssize_t my_class_attr_show(struct device *dev, 
                                       struct device_attribute *attr, 
                                       char *buf)
{
    // Extract the minor number from the device structure
    int minor = MINOR(dev->devt); 
    
    // Check actual used data size (or use sizeof(kernel_buffer[minor]) for allocated size)
    size_t memory_used = strlen(kernel_buffer[minor]); 

    // Print the size into the sysfs buffer
    return sysfs_emit(buf, "Allocated: %d bytes | Used: %zu bytes\n", BUF_SIZE, memory_used);
}

/* Define the device attribute macro (creates a dev_attr_memory_usage structure)
	Arguments:
		memory_usage -> name of the attribute
		0444         -> permissions (read-only)
		my_class_attr_show -> show function
		NULL         -> store function (not needed for read-only)
*/
static DEVICE_ATTR(memory_usage, 0444, my_class_attr_show, NULL);

static int __init my_init(void)
{
    int ret;
    int i;

    ret = alloc_chrdev_region(&dev_num,
                              0,
                              NUM_DEVICES,
                              "mychar");

    if (ret)
        return ret;

    cdev_init(&my_cdev, &my_fops);

    ret = cdev_add(&my_cdev,
                   dev_num,
                   NUM_DEVICES);

    if (ret) {
        unregister_chrdev_region(dev_num,
                                 NUM_DEVICES);
        return ret;
    }

    my_class = class_create("mychar_class");

    if (IS_ERR(my_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num,
                                 NUM_DEVICES);
        return PTR_ERR(my_class);
    }

    for (i = 0; i < NUM_DEVICES; i++) {

        my_device[i] =
            device_create(my_class,
                          NULL,
                          MKDEV(MAJOR(dev_num), i),    /* MKDEV - create device number from major and minor */
                          NULL,
                          "mychar%d",
                          i);

        if (IS_ERR(my_device[i])) {

            while (--i >= 0)
                device_destroy(my_class,
                               MKDEV(MAJOR(dev_num), i));

            class_destroy(my_class);
            cdev_del(&my_cdev);
            unregister_chrdev_region(dev_num,
                                     NUM_DEVICES);

            return PTR_ERR(my_device[i]);
        }

		/*  
			Create the sysfs file for this minor device 
			It is used to show the memory usage of the device in /sys/class/mychar_class/mycharX/memory_usage
		*/
        ret = device_create_file(my_device[i], &dev_attr_memory_usage);
        if (ret) {
            pr_err("Failed to create sysfs file for mychar%d\n", i);
            // Optional: Handle error / clean up
        }
    }

    pr_info("Major=%d\n", MAJOR(dev_num));

    return 0;
}

static void __exit my_exit(void)
{
    int i;

	for (i = 0; i < NUM_DEVICES; i++) {
        // Remove sysfs file 
        device_remove_file(my_device[i], &dev_attr_memory_usage);

        device_destroy(my_class, MKDEV(MAJOR(dev_num), i));
    }
    class_destroy(my_class);

    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num,
                             NUM_DEVICES);

    pr_info("Driver removed\n");
}

/* module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Multiple Minor Character Driver"); */