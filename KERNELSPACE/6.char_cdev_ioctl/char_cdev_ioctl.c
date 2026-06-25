#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "mychar"
#define CLASS_NAME  "mychar_class"

struct my_cfg {
    int id;
    int speed;
    char name[32];
}my_cfg;

/* IOCTL Commands */
#define MY_IOCTL_MAGIC 'A'

#define SET_VALUE _IOW(MY_IOCTL_MAGIC, 1, struct my_cfg )
#define GET_VALUE _IOR(MY_IOCTL_MAGIC, 2, struct my_cfg )

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;

/* Open */
static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mychar opened\n");
    return 0;
}

/* Release */
static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mychar closed\n");
    return 0;
}

/* Read */
static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    return 0;
}

/* Write */
static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    return len;
}

/*
*		Interview takeaway
*
*		"Give one practical use of ioctl."
*		
*		Answer:
*		
*			UART baudrate configuration
*			CAN bitrate configuration
*			Get driver statistics
*			Reset device
*			Enable/disable loopback mode
*			Hardware-specific commands that don't fit read/write
*/
static long my_ioctl(struct file *file,
                     unsigned int cmd,
                     unsigned long arg)
{
    switch (cmd) {

    case SET_VALUE:

        if (copy_from_user(&my_cfg,
                           (struct my_cfg __user *)arg,
                           sizeof(my_cfg)))
            return -EFAULT;

        pr_info("SET_VALUE = id: %d, speed: %d, name: %s\n", my_cfg.id, my_cfg.speed, my_cfg.name);
        break;

    case GET_VALUE:

        if (copy_to_user((struct my_cfg __user *)arg,
                         &my_cfg,
                         sizeof(my_cfg)))
            return -EFAULT;

        pr_info("GET_VALUE = id: %d, speed: %d, name: %s\n", my_cfg.id, my_cfg.speed, my_cfg.name);
        break;

    default:
        pr_err("Unknown IOCTL cmd\n");
        return -EINVAL;
    }

    return 0;
}

static struct file_operations my_fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .read           = my_read,
    .write          = my_write,
    .unlocked_ioctl = my_ioctl,
};

static int __init my_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num,
                              0,
                              1,
                              DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&my_cdev, &my_fops);

    ret = cdev_add(&my_cdev,
                   dev_num,
                   1);
    if (ret)
        goto err_cdev;

    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        goto err_class;
    }

    device_create(my_class,
                  NULL,
                  dev_num,
                  NULL,
                  "mychar0");

    pr_info("Driver loaded\n");
    pr_info("Major=%d Minor=%d\n",
            MAJOR(dev_num),
            MINOR(dev_num));

    return 0;

err_class:
    cdev_del(&my_cdev);

err_cdev:
    unregister_chrdev_region(dev_num, 1);

    return ret;
}

static void __exit my_exit(void)
{
    device_destroy(my_class, dev_num);

    class_destroy(my_class);

    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num, 1);

    pr_info("Driver unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("IOCTL Example Driver");