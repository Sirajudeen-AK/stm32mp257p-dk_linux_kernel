/*
Driver poll() flowchart:

			Userspace
				|
			poll()
				|
				v
			Driver poll()
				|
				+--> data available ?
						|
					------------
					|			|
				   YES         NO
					|			|
					v			|	
			return READY	 	|
								|
								v
						sleep in waitqueue

			write()/ISR
				|
				v
			wake_up_interruptible()

			poll() returns
			read() executes

*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define DEVICE_NAME "mychar"

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;

static char buffer[100];
static int data_available;

static wait_queue_head_t my_wq;

static int my_open(struct inode *inode,
                   struct file *file)
{
    pr_info("opened the Process-id %d\n", current->pid);
    return 0;
}

static int my_release(struct inode *inode,
                      struct file *file)
{
    pr_info("closed the Process-id %d\n", current->pid);
    return 0;
}

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    int data_len;

    if (!data_available)
        return 0;

    data_len = strlen(buffer);

    if (len > data_len)
        len = data_len;

    if (copy_to_user(buf, buffer, len))
        return -EFAULT;

    data_available = 0;

    return len;
}

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    if (len > sizeof(buffer) - 1)
        len = sizeof(buffer) - 1;

    if (copy_from_user(buffer, buf, len))
        return -EFAULT;

    buffer[len] = '\0';

    data_available = 1;

	/*
		Argument 
		1: wait queue head
		2: number of processes to wake up
	*/
	wake_up_interruptible_nr(&my_wq, 1);

    pr_info("Process ID %d data received: %s\n", current->pid, buffer);

    return len;
}

static __poll_t my_poll(struct file *f,
                        poll_table *wait)
{
	pr_info("poll entered pid=%d\n", current->pid);

    poll_wait(f, &my_wq, wait);

    if (data_available){
		pr_info("poll ready pid=%d\n", current->pid);
		return POLLIN | POLLRDNORM;
	}
	
	pr_info("poll exited pid=%d and data_available=%d\n", current->pid, data_available);

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .poll = my_poll,
};

static int __init my_init(void)
{
	init_waitqueue_head(&my_wq);

    alloc_chrdev_region(&dev_num,
                        0,
                        1,
                        DEVICE_NAME);

    cdev_init(&my_cdev, &fops);

    cdev_add(&my_cdev,
             dev_num,
             1);

    my_class = class_create(DEVICE_NAME);

    device_create(my_class,
                  NULL,
                  dev_num,
                  NULL,
                  DEVICE_NAME);

    pr_info("loaded major=%d\n",
            MAJOR(dev_num));

    return 0;
}

static void __exit my_exit(void)
{
    device_destroy(my_class,
                   dev_num);

    class_destroy(my_class);

    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num,
                             1);

    pr_info("unloaded\n");
}

module_init(my_init);
module_exit(my_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("");