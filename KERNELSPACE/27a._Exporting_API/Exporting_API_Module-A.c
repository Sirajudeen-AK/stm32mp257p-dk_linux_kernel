
/*
	Module A
	↓
	Exports APIs
	↓
	Module B
	↓
	Calls Module A APIs

Exactly how many Linux kernel subsystems work.
	Examples:
		Network Driver → PHY Driver
		CAN Driver → Protocol Driver
		USB Core → USB Drivers
		V4L2 Core → Camera Drivers
		DRM Core → Display Drivers


How EXPORT_SYMBOL Works
Internally
		Module A
		↓
		EXPORT_SYMBOL()
		↓
		Kernel Symbol Table
		↓
		Module B
		↓
		Symbol Lookup
		↓
		Function Address
		↓
		Call

No copying.
No IPC.
No sockets.

Just a function pointer resolved by the kernel loader.

*/

#include <linux/module.h>
#include <linux/kernel.h>

int shared_counter = 0;

void increment_counter(void)
{
    shared_counter++;
    pr_info("Module A : counter=%d\n", shared_counter);
}

int get_counter(void)
{
    return shared_counter;
}

EXPORT_SYMBOL(increment_counter);
EXPORT_SYMBOL(get_counter);

static int __init module_a_init(void)
{
    pr_info("Module A Loaded\n");
    return 0;
}

static void __exit module_a_exit(void)
{
    pr_info("Module A Unloaded\n");
}

module_init(module_a_init);
module_exit(module_a_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Exporting APIs Between Kernel Modules - Module A");

