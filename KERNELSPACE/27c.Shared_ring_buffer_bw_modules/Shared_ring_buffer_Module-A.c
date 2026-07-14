#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#define MAX_PACKET_SIZE    64
#define RING_SIZE          8

struct ring_buffer {

    char data[RING_SIZE][MAX_PACKET_SIZE];

    unsigned int head;

    unsigned int tail;
};

static struct ring_buffer *ring;

int ring_push(const char *msg);
int ring_pop(char *msg);
struct ring_buffer *get_ring(void);

struct ring_buffer *get_ring(void)
{
    return ring;
}
EXPORT_SYMBOL(get_ring);

int ring_push(const char *msg)
{
    unsigned int next;

    next = (ring->head + 1) % RING_SIZE;	// condition to make it circular 

    if (next == ring->tail)
        return -ENOSPC;

    strscpy(ring->data[ring->head],
            msg,
            MAX_PACKET_SIZE);

    ring->head = next;

    return 0;
}
EXPORT_SYMBOL(ring_push);

int ring_pop(char *msg)
{
    if (ring->head == ring->tail)
        return -ENOENT;

    strscpy(msg,
            ring->data[ring->tail],
            MAX_PACKET_SIZE);

    ring->tail =
        (ring->tail + 1) % RING_SIZE;

    return 0;
}
EXPORT_SYMBOL(ring_pop);

static int __init module_a_init(void)
{
	pr_info("Module A loaded"); 

    ring = kzalloc(sizeof(*ring), GFP_KERNEL);

    if (!ring)
        return -ENOMEM;

    pr_info("Ring buffer Created\n");

    return 0;
}

static void __exit module_a_exit(void)
{
    kfree(ring);

    pr_info("Ring buffer Removed\n");

	pr_info("Module A unloaded");
}

module_init(module_a_init);
module_exit(module_a_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Shared ring buffer Between Kernel Modules - Module A");

