#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mutex.h>

typedef void (*event_callback_t)(const char *);

struct callback_node {

    event_callback_t cb;

    struct list_head list;
};

static LIST_HEAD(callback_list);

static DEFINE_MUTEX(callback_lock);

int register_event_callback(event_callback_t cb)
{
    struct callback_node *node;

    node = kmalloc(sizeof(*node), GFP_KERNEL);
    if (!node)
        return -ENOMEM;

    node->cb = cb;

    mutex_lock(&callback_lock);

    list_add_tail(&node->list,
                  &callback_list);

    mutex_unlock(&callback_lock);

    pr_info("Callback Registered\n");

    return 0;
}

EXPORT_SYMBOL(register_event_callback);

void unregister_event_callback(event_callback_t cb)
{
    struct callback_node *node;
    struct callback_node *tmp;

    mutex_lock(&callback_lock);

    list_for_each_entry_safe(node,
                             tmp,
                             &callback_list,
                             list) {

        if (node->cb == cb) {

            list_del(&node->list);

            kfree(node);

            break;
        }
    }

    mutex_unlock(&callback_lock);

    pr_info("Callback Unregistered\n");
}

EXPORT_SYMBOL(unregister_event_callback);

void notify_event(const char *msg)
{
    struct callback_node *node;

    mutex_lock(&callback_lock);

    list_for_each_entry(node,
                        &callback_list,
                        list) {

        node->cb(msg);
    }

    mutex_unlock(&callback_lock);
}

EXPORT_SYMBOL(notify_event);

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
MODULE_DESCRIPTION("Multiple Callback registartion Module - Module A");

