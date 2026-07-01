#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/mutex.h>

#define DEVICE_NAME "list_demo"
#define BUF_SIZE    128

struct student
{
    int id;
    char name[32];
    struct list_head list;
};

static LIST_HEAD(student_list);
static DEFINE_MUTEX(list_lock);

static dev_t devno;
static struct cdev list_cdev;
static struct class *list_class;
static struct device *list_device;

static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "list_demo opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "list_demo closed\n");
    return 0;
}

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    char kbuf[BUF_SIZE];
    struct student *node;

    if (len >= BUF_SIZE)
        return 0;

    if (copy_from_user(kbuf, buf, len))
        return 0;

    kbuf[len] = '\0';

    node = kzalloc(sizeof(*node), GFP_KERNEL);
    if (!node)
        return 0;

    if (sscanf(kbuf, "%d %31s", &node->id, node->name) != 2)
    {
        kfree(node);
        return 0;
    }

    mutex_lock(&list_lock);

    list_add_tail(&node->list, &student_list);

    mutex_unlock(&list_lock);

    printk(KERN_INFO "Added : id=%d name=%s\n",
           node->id,
           node->name);

    return len;
}

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    char kbuf[512];
    int pos = 0;

    struct student *node;

    if (*off != 0)
        return 0;

    mutex_lock(&list_lock);

    list_for_each_entry(node,
                        &student_list,
                        list)
    {
        pos += scnprintf(kbuf + pos,
                         sizeof(kbuf) - pos,
                         "ID=%d Name=%s\n",
                         node->id,
                         node->name);

        if (pos >= sizeof(kbuf))
            break;
    }

    mutex_unlock(&list_lock);

    if (copy_to_user(buf, kbuf, pos))
        return -EFAULT;

    *off += pos;

    return pos;
}

static long my_ioctl(struct file *file,
                     unsigned int cmd,
                     unsigned long arg)
{
    struct student *node;
    struct student *tmp;
	#define DELETE_NODE 1

    switch (cmd)
    {
    case DELETE_NODE:

        mutex_lock(&list_lock);

        list_for_each_entry_safe(node,
                                 tmp,
                                 &student_list,
                                 list)
        {
            if (node->id == arg)
            {
                list_del(&node->list);

                printk(KERN_INFO
                       "Deleted ID=%d\n",
                       node->id);

                kfree(node);

                mutex_unlock(&list_lock);

                return 0;
            }
        }

        mutex_unlock(&list_lock);

        return 1; // not valid ID
    }

    return 2; // not valid control ID
}

static struct file_operations fops =
{
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .read           = my_read,
    .write          = my_write,
    .unlocked_ioctl = my_ioctl,
};

static int __init list_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&devno,
                              0,
                              1,
                              DEVICE_NAME);

    if (ret)
        return ret;

    cdev_init(&list_cdev,
              &fops);

    ret = cdev_add(&list_cdev,
                   devno,
                   1);

    if (ret)
        goto err_chrdev;

    list_class = class_create(DEVICE_NAME);

    if (IS_ERR(list_class))
    {
        ret = PTR_ERR(list_class);
        goto err_cdev;
    }

    list_device = device_create(list_class,
                                NULL,
                                devno,
                                NULL,
                                DEVICE_NAME);

    if (IS_ERR(list_device))
    {
        ret = PTR_ERR(list_device);
        goto err_class;
    }

    printk(KERN_INFO
           "Loaded Major=%d Minor=%d\n",
           MAJOR(devno),
           MINOR(devno));

    return 0;

err_class:
    class_destroy(list_class);

err_cdev:
    cdev_del(&list_cdev);

err_chrdev:
    unregister_chrdev_region(devno,1);

    return ret;
}

static void __exit list_exit(void)
{
    struct student *node;
    struct student *tmp;

    mutex_lock(&list_lock);

    list_for_each_entry_safe(node,
                             tmp,
                             &student_list,
                             list)
    {
        list_del(&node->list);
        kfree(node);
    }

    mutex_unlock(&list_lock);

    device_destroy(list_class, devno);

    class_destroy(list_class);

    cdev_del(&list_cdev);

    unregister_chrdev_region(devno, 1);

    printk(KERN_INFO "Driver Removed\n");
}

module_init(list_init);
module_exit(list_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Kernel Linked List Demo");