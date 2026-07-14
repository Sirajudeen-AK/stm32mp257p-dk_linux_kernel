#include <linux/module.h>
#include <linux/notifier.h>

/*
This creates
	Notifier Head
	↓
	Empty Callback List
*/
BLOCKING_NOTIFIER_HEAD(my_chain);

int register_my_notifier(struct notifier_block *nb)
{
    return blocking_notifier_chain_register(
                &my_chain,
                nb);
}
EXPORT_SYMBOL(register_my_notifier);

int unregister_my_notifier(struct notifier_block *nb)
{
    return blocking_notifier_chain_unregister(
                &my_chain,
                nb);
}
EXPORT_SYMBOL(unregister_my_notifier);

void send_event(unsigned long event, void *data)
{
    blocking_notifier_call_chain(
            &my_chain,
            event,
            data);
}
EXPORT_SYMBOL(send_event);

static int __init module_a_init(void)
{
    pr_info("Notifier Framework Loaded\n");

    return 0;
}

static void __exit module_a_exit(void)
{
    pr_info("Notifier Framework Removed\n");
}

module_init(module_a_init);
module_exit(module_a_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Linux notifier chain - Module A");

