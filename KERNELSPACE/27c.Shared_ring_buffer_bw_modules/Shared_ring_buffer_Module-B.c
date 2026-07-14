#include <linux/module.h>
#include <linux/kernel.h>

extern int ring_push(const char *);
extern int ring_pop(char *);

static int __init module_b_init(void)
{
    char buf[64];
	pr_info("Module B loaded");

    ring_push("Packet A");

    ring_push("Packet B");

    ring_push("Packet C");

    while (!ring_pop(buf))
        pr_info("Received : %s\n", buf);

    return 0;
}

static void __exit module_b_exit(void)
{
	pr_info("Module B unloaded");
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Shared ring buffer Between Kernel Modules - Module B");