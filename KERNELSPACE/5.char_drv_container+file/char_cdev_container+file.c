#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychar"

struct my_device {
    struct cdev cdev;
    char buffer[100];
    int size;
    int id;
};

static dev_t dev_num;
static struct class *my_class;

static struct my_device devices[4];

/*
*		Interview Question
*		
*		Why use:
*		file->private_data
*		
*		Answer:
*		
*		.open() executes once when the file is opened. We store the device context in file->private_data. 
*		Later .read(), .write(), and .release() can directly access the correct device instance without 
*		searching again using inode/minor numbers.
*		
*/
static int my_open(struct inode *inode, struct file *file)
{
    struct my_device *dev;

    dev = container_of(inode->i_cdev,
                       struct my_device,
                       cdev);

    file->private_data = dev;

    pr_info("Opened minor=%d\n", dev->id);

    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    struct my_device *dev = file->private_data;

    pr_info("Closed minor=%d\n", dev->id);

    return 0;
}

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    struct my_device *dev = file->private_data;

    if (len > sizeof(dev->buffer))
        len = sizeof(dev->buffer);

    if (copy_from_user(dev->buffer, buf, len))
        return -EFAULT;

    dev->size = len;

    pr_info("Minor %d received: %.*s",
            dev->id,
            (int)len,
            dev->buffer);

    return len;
}

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    struct my_device *dev = file->private_data;

    if (*off >= dev->size)
        return 0;

    if (len > dev->size - *off)
        len = dev->size - *off;

    if (copy_to_user(buf,
                     dev->buffer + *off,
                     len))
        return -EFAULT;

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

static int __init my_init(void)
{
    int i;

    alloc_chrdev_region(&dev_num, 0, 4, DEVICE_NAME);

    my_class = class_create(DEVICE_NAME);

    for (i = 0; i < 4; i++) {

        devices[i].id = i;

        cdev_init(&devices[i].cdev, &fops);

        cdev_add(&devices[i].cdev,
                 MKDEV(MAJOR(dev_num), i),
                 1);

        device_create(my_class,
                      NULL,
                      MKDEV(MAJOR(dev_num), i),
                      NULL,
                      "mychar%d",
                      i);
    }

    pr_info("Loaded Major=%d\n", MAJOR(dev_num));

    return 0;
}

static void __exit my_exit(void)
{
    int i;

    for (i = 0; i < 4; i++) {

        device_destroy(my_class,
                       MKDEV(MAJOR(dev_num), i));

        cdev_del(&devices[i].cdev);
    }

    class_destroy(my_class);

    unregister_chrdev_region(dev_num, 4);

    pr_info("Unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Multiple Minor Character Driver");