#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychar"
#define BUF_SIZE    100

static int major;
static char kernel_buffer[BUF_SIZE];

static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mychar opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mychar closed\n");
    return 0;
}

/*
*		User-space write() flow:
*		echo
*		 ↓
*		write()
*		 ↓
*		my_write()
*		 ↓
*		copy_from_user()
*		 ↓
*		kernel_buffer
*/
static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *offset)
{
    if (len > BUF_SIZE - 1)
        len = BUF_SIZE - 1;

    if (copy_from_user(kernel_buffer, buf, len))
        return -EFAULT;

    kernel_buffer[len] = '\0';

    printk(KERN_INFO "Received: %s\n", kernel_buffer);

    return len;
}

/*
*		User-space read() flow:
*		cat
*		 ↓
*		read()
*		 ↓
*		my_read()
*		 ↓
*		copy_to_user()
*		 ↓
*		user_buffer
*/
static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *offset)
{
    int data_len = strlen(kernel_buffer);

    if (*offset >= data_len)
        return 0;

    if (len > data_len - *offset)
        len = data_len - *offset;

    if (copy_to_user(buf,
                     kernel_buffer + *offset,
                     len))
        return -EFAULT;

    *offset += len;

    return len;
}

static const struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

static int __init char_init(void)
{
    major = register_chrdev(0,
                            DEVICE_NAME,
                            &my_fops);

    if (major < 0)
        return major;

    printk(KERN_INFO "mychar loaded major=%d\n",
           major);

    return 0;
}

static void __exit char_exit(void)
{
    unregister_chrdev(major,
                      DEVICE_NAME);

    printk(KERN_INFO "mychar unloaded\n");
}


/*
*	Interview Question
*	
*	Why can't kernel do this?
*		memcpy(user_buf, kernel_buf, len);
*	
*	instead of
*		copy_to_user()
*	
*	Answer:
*		user_buf belongs to userspace.
*	
*		Kernel cannot trust userspace pointers.
*	
*		Pointer may be:
*		- NULL
*		- Invalid
*		- Unmapped
*		- Pointing to another process
*		- Pointing to kernel space
*		
*		copy_to_user() validates access and handles faults.

*/
module_init(char_init);
module_exit(char_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("STM32 cdev driver");