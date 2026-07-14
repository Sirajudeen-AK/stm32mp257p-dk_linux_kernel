#include <linux/module.h>
#include <linux/kernel.h>

extern int register_event_callback(void (*cb)(const char *));
extern void unregister_event_callback(void (*cb)(const char *));


static void callback_C(const char *msg)
{
    pr_info("Module C : %s\n", msg);
}

static int __init module_c_init(void)
{
    register_event_callback(callback_C);

    return 0;
}

static void __exit module_c_exit(void)
{
    unregister_event_callback(callback_C);
}

module_init(module_c_init);
module_exit(module_c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Multiple Callback registartion Module - Module C");