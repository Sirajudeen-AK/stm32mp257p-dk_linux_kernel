#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

/*
		insmod mychar.ko
				|
				V
		module_init()
				|
				V
		register_chrdev()
				|
				V
		Kernel allocates major number
				|
				V
		Appears in /proc/devices ( not in /dev/ )
		
*/

/*
*	interviewer asks:
*		I called register_chrdev(). Why don't I see /dev/mychar?
*	
*	Answer:
*		register_chrdev() only registers the driver and allocates a major number. It does not create the device node. 
*		We need mknod or device_create() to create /dev/mychar.
*	
*
*	What is struct file_operations?
*	Answer:
*		It is a table of callback functions that VFS invokes when user-space performs operations on a device file.*
*
*/
#define DEVICE_NAME "mychar"

static int major;

static const struct file_operations my_fops = {
        .owner = THIS_MODULE,
};

static int __init mychar_init(void)
{
	/*
	1st Parameter: 0 means kernel will allocate a major number dynamically
	               or else we can specify a major number (e.g., 240) if we want to use a static major number.
		Why dynamic major is preferred?
			- Hardcoded major may clash with another driver.
			- Dynamic allocation avoids conflicts.
			- Production drivers usually use dynamic major allocation.			   
	*/
    major = register_chrdev(0,
                            DEVICE_NAME,
                            &my_fops);

    if (major < 0) {
        pr_err("register_chrdev failed\n");
        return major;
    }

    pr_info("Major number = %d\n", major);

    return 0;
}

static void __exit mychar_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);

    pr_info("Driver removed\n");
}

/* module_init(mychar_init);
module_exit(mychar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Simple Character Driver"); */