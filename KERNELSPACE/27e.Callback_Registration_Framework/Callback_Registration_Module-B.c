#include <linux/module.h>
#include <linux/kernel.h>

extern int register_event_callback(
            void (*cb)(const char *));

extern void unregister_event_callback(void);

extern void notify_event(const char *);

static void my_callback(const char *msg)
{
    pr_info("Callback Received : %s\n",
            msg);
}

static int __init module_b_init(void)
{
    register_event_callback(my_callback);

    notify_event("Packet Received");

    notify_event("CAN Frame");

    notify_event("Interrupt");

    return 0;
}

static void __exit module_b_exit(void)
{
    unregister_event_callback();
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Callback registartion Module - Module B");