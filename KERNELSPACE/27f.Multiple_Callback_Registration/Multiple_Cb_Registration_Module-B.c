#include <linux/module.h>
#include <linux/kernel.h>

extern int register_event_callback(void (*cb)(const char *));
extern void unregister_event_callback(void (*cb)(const char *));
extern void notify_event(const char *);


static void callback_B(const char *msg)
{
    pr_info("Module B : %s\n", msg);
}

static int __init module_b_init(void)
{
    register_event_callback(callback_B);

    return 0;
}

static void __exit module_b_exit(void)
{
    unregister_event_callback(callback_B);
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Multiple Callback registartion Module - Module B");