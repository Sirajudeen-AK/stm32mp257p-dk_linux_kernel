#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

/*
Difference between register_chrdev() and cdev_add()?
	register_chrdev():
		Old API
		One function does everything

	cdev_add():
		Modern API
		More flexible
		Supports multiple minors cleanly
		Used in real drivers
*/

#define BUF_SIZE 100

static dev_t dev_num;
static struct cdev my_cdev;

static char kernel_buffer[BUF_SIZE];

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mychar opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mychar closed\n");
    return 0;
}

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

    *off += len;

    return len;
}

static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
};
/*
OLD METHOD:
		register_chrdev()
			|
			V
		One API does everything

NEW METHOD:
		insmod
		  |
		  V
		alloc_chrdev_region()
		  |
		  V
		Major = 510
		Minor = 0
		  |
		  V
		cdev_init()
		  |
		  V
		Attach my_fops
		  |
		  V
		cdev_add()
		  |
		  V
		Kernel registers device
*/
static int __init my_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num,
                              0,
                              1,
                              "mychar");

    if (ret < 0)
        return ret;

    cdev_init(&my_cdev, &my_fops);

    ret = cdev_add(&my_cdev,
                   dev_num,
                   1);

    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    pr_info("Major=%d Minor=%d\n",
            MAJOR(dev_num),
            MINOR(dev_num));

    return 0;
}

static void __exit my_exit(void)
{
    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num,
                             1);

    pr_info("Driver removed\n");
}

/* module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL"); */