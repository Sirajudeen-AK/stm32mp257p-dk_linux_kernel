/*
Data flow in char device driver with waiting queue

			cat /dev/mychar0
			      |
			      v
			   read()
			      |
			      v
			No data?
			      |
			      +----YES---->
			                 Sleep
			                    |
			                    |
			              wait_queue
			                    |
			                    |
			echo hello > /dev/mychar0
			                    |
			                    |
			             wake_up()
			                    |
			                    v
			               read() wakes
			                    |
			                    v
			            returns data
*/

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

static char buffer[100];
static int data_available;

/* Declare and initialize a wait queue head */
static DECLARE_WAIT_QUEUE_HEAD(my_wq);

static int my_open(struct inode *inode,
                   struct file *file)
{
    pr_info("opened\n");
    return 0;
}

static int my_release(struct inode *inode,
                      struct file *file)
{
    pr_info("closed\n");
    return 0;
}

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    int ret;
    int data_len;

    pr_info("read waiting...\n");

	/*
	Difference between wait_event and wait_event_interruptible?
	
		wait_event() - uninterruptible sleep
		wait_event_interruptible() - interruptible sleep ( ctrl + C -> SIGINT )

		Interruptible sleep is preferred in most cases, because it allows the process to be interrupted by signals.	
	*/
    ret = wait_event_interruptible(
                my_wq,
                data_available);

    if (ret)
        return ret;

    data_len = strlen(buffer);

    if (len > data_len)
        len = data_len;

    if (copy_to_user(buf, buffer, len))
        return -EFAULT;

    data_available = 0;

    pr_info("read completed\n");

    return len;
}

/*
			Why important for interviews?

				Because UART, CAN, Ethernet, RPMsg, sensor drivers all use this exact pattern:

				read()
					|
				wait_event_interruptible()

				ISR/RX callback
					|
				wake_up_interruptible()

*/
static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    if (len > sizeof(buffer)-1)
        len = sizeof(buffer)-1;

    if (copy_from_user(buffer, buf, len))
        return -EFAULT;

    buffer[len] = '\0';

    data_available = 1;

/*
		Where is wake_up_interruptible() called in real drivers?

		Usually:

			ISR
			Bottom half
			Workqueue
			RX callback

		Example CAN:
			CAN frame received
				|
			  ISR
				|
			store frame
				|
			wake_up_interruptible()
*/
	/* Wake up any processes waiting for data */
    wake_up_interruptible(&my_wq);

    pr_info("wakeup sent\n");

    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init my_init(void)
{
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
                  "mychar0");

    pr_info("loaded major=%d\n",
            MAJOR(dev_num));

    return 0;
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