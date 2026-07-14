#include <linux/module.h>
#include <linux/notifier.h>

extern int register_my_notifier(
            struct notifier_block *);

extern int unregister_my_notifier(
            struct notifier_block *);

extern void send_event(
            unsigned long,
            void *);


static int callback2(
        struct notifier_block *nb,
        unsigned long event,
        void *data)
{
    pr_info("Module C Event=%lu Message=%s\n",
            event,
            (char *)data);

    return NOTIFY_OK;
}

static struct notifier_block nb2 = {

    .notifier_call = callback2,

    .priority = 20,				//high priority compare to Module-B so Module-C executes first
};


static int __init module_b_init(void)
{
    register_my_notifier(&nb2);

    send_event(1, "Packet");

    send_event(2, "CAN");

    send_event(3, "IRQ");

    return 0;
}

static void __exit module_b_exit(void)
{
    unregister_my_notifier(&nb2);
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Linux notifier chain - Module C");