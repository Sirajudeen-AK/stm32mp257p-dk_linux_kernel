
/*
Memory Diagram
              Module A

          callback pointer
                │
                ▼
         my_callback()
                ▲
                │
              Module B

Module A never knows who registered.

It simply executes
	callback(...)

*/


#include <linux/module.h>
#include <linux/kernel.h>

typedef void (*event_callback_t)(const char *);

static event_callback_t callback;

int register_event_callback(event_callback_t cb)
{
    if (callback)
        return -EBUSY; // try to register 2nd time it will fail

    callback = cb;

    pr_info("Callback Registered\n");

    return 0;
}

EXPORT_SYMBOL(register_event_callback);

void unregister_event_callback(void)
{
	/*
	if not assign to NULL
		NULL Pointer Dereference
		Kernel Oops
	*/
    callback = NULL;

    pr_info("Callback Unregistered\n");
}

EXPORT_SYMBOL(unregister_event_callback);

void notify_event(const char *msg)
{
    if (callback)
        callback(msg);
}

EXPORT_SYMBOL(notify_event);

static int __init module_a_init(void)
{
    pr_info("Module A Loaded\n");

    return 0;
}

static void __exit module_a_exit(void)
{
    callback = NULL;

    pr_info("Module A Unloaded\n");
}

module_init(module_a_init);
module_exit(module_a_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Callback registartion Module - Module A");

