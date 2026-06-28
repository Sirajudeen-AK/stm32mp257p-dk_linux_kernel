#include <linux/init.h>
#include <linux/module.h>


/*
	load module (insmod param.ko number=20)
		↓
	parse parameters 
		↓
	assign values
		↓
	call init function (param_init)
*/

static int number = 10;

/* module_param -> to pass parameters to the module at the time of loading
* if not using, default value will be used 
* permissions: 0644 -> read/write for owner, read for group and others
*
* Kernel ↔ Sysfs ↔ Userspace
* cat /sys/module/par/parameters/number
*
*/
module_param(number, int, 0644);

static int __init param_init(void)
{
    pr_info("number = %d\n", number);
    return 0;
}

static void __exit param_exit(void)
{
    pr_info("param module exit\n");
}

module_init(param_init);
module_exit(param_exit);

MODULE_LICENSE("GPL");