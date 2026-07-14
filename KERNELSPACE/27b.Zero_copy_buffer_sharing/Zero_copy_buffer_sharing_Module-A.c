
/*
		Module A

		kmalloc()
		↓
		shared_buffer
		↓
		EXPORT_SYMBOL(get_shared_buffer)
		↓
		Module B
		↓
		char *ptr
		↓
		Read/Write

*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#define BUFFER_SIZE 256

static char *shared_buffer;

char *get_shared_buffer(void)
{
    return shared_buffer;
}
EXPORT_SYMBOL(get_shared_buffer);

size_t get_shared_buffer_size(void)
{
    return BUFFER_SIZE;
}
EXPORT_SYMBOL(get_shared_buffer_size);

static int __init module_a_init(void)
{
    shared_buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!shared_buffer)
        return -ENOMEM;

    strcpy(shared_buffer, "Hello from Module A");

    pr_info("Module A Loaded\n");
    pr_info("Buffer Virtual Address : %px\n", shared_buffer);
    pr_info("Buffer Content : %s\n", shared_buffer);

    return 0;
}

static void __exit module_a_exit(void)
{
    kfree(shared_buffer);

    pr_info("Module A Unloaded\n");
}

module_init(module_a_init);
module_exit(module_a_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Zero Copy buffer sharing - Module A");

