#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define DEVICE_NAME "mychar"

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;

static char kernel_buffer[100];
static int data_available;

static wait_queue_head_t my_wq;

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

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *offset)
{
    int bytes;

    pr_info("Reader sleeping...\n");

    wait_event_interruptible(my_wq, data_available);

    pr_info("Reader woke up\n");

    bytes = min(len, strlen(kernel_buffer));

    if (copy_to_user(buf, kernel_buffer, bytes))
        return -EFAULT;

    data_available = 0;

    return bytes;
}

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *offset)
{
    int bytes;

    bytes = min(len, sizeof(kernel_buffer) - 1);

    if (copy_from_user(kernel_buffer, buf, bytes))
        return -EFAULT;

    kernel_buffer[bytes] = '\0';

    pr_info("Received: %s\n", kernel_buffer);

    data_available = 1;

    wake_up_interruptible(&my_wq);

    return len;
}

static struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

static int __init my_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&my_cdev, &my_fops);

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret)
        goto err_chrdev;

    my_class = class_create(DEVICE_NAME);
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        goto err_cdev;
    }

    device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);

    init_waitqueue_head(&my_wq);

    data_available = 0;

    pr_info("loaded major=%d minor=%d\n",
            MAJOR(dev_num),
            MINOR(dev_num));

    return 0;

err_cdev:
    cdev_del(&my_cdev);

err_chrdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit my_exit(void)
{
    device_destroy(my_class, dev_num);
    class_destroy(my_class);

    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num, 1);

    pr_info("unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Blocking Read Driver");