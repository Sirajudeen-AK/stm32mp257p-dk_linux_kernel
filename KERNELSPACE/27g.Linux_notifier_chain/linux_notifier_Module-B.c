#include <linux/module.h>
#include <linux/notifier.h>

extern int register_my_notifier(
            struct notifier_block *);

extern int unregister_my_notifier(
            struct notifier_block *);

extern void send_event(
            unsigned long,
            void *);


static int my_callback(
        struct notifier_block *nb,
        unsigned long event,
        void *data)
{
    pr_info("Module B Event=%lu Message=%s\n",
            event,
            (char *)data);

    return NOTIFY_OK;
}

static struct notifier_block my_nb = {

    .notifier_call = my_callback,

    .priority = 10,	//low priority compare to Module-C so Module-C executes first
};


static int __init module_b_init(void)
{
    register_my_notifier(&my_nb);

    // send_event(1, "Packet");

    // send_event(2, "CAN");

    // send_event(3, "IRQ");

    return 0;
}

static void __exit module_b_exit(void)
{
    unregister_my_notifier(&my_nb);
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Linux notifier chain - Module B");