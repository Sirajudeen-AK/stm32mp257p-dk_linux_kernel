#include <linux/module.h>
#include <linux/kernel.h>

extern void notify_event(const char *);

static int __init module_d_init(void)
{
    notify_event("Packet Received");

	notify_event("CAN Frame");
	
	notify_event("Interrupt");

    return 0;
}

static void __exit module_d_exit(void)
{
}

module_init(module_d_init);
module_exit(module_d_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Multiple Callback registartion Module - Module D");