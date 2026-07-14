#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/string.h>

#define RING_SIZE       8
#define MAX_PACKET      64

struct ring_buffer {

    char data[RING_SIZE][MAX_PACKET];

    unsigned int head;

    unsigned int tail;

    spinlock_t lock;

    wait_queue_head_t read_queue;
};

static struct ring_buffer *ring;

int ring_push(const char *msg);
int ring_pop(char *msg);

EXPORT_SYMBOL(ring_push);
EXPORT_SYMBOL(ring_pop);

int ring_push(const char *msg)
{
    unsigned long flags;
    unsigned int next;

    spin_lock_irqsave(&ring->lock, flags);

    next = (ring->head + 1) % RING_SIZE;

    if (next == ring->tail) {

        spin_unlock_irqrestore(&ring->lock, flags);

        return -ENOSPC;
    }

    strscpy(ring->data[ring->head],
            msg,
            MAX_PACKET);

    ring->head = next;

    spin_unlock_irqrestore(&ring->lock, flags);

    wake_up_interruptible(&ring->read_queue);

    return 0;
}

int ring_pop(char *msg)
{
    unsigned long flags;

    wait_event_interruptible(
        ring->read_queue,
        ring->head != ring->tail);

    spin_lock_irqsave(&ring->lock, flags);

    strscpy(msg,
            ring->data[ring->tail],
            MAX_PACKET);

    ring->tail =
        (ring->tail + 1) % RING_SIZE;

    spin_unlock_irqrestore(&ring->lock, flags);

    return 0;
}

static int __init module_a_init(void)
{
    ring = kzalloc(sizeof(*ring), GFP_KERNEL);

    if (!ring)
        return -ENOMEM;

    spin_lock_init(&ring->lock);

    init_waitqueue_head(&ring->read_queue);

    pr_info("Ring Initialized\n");

    return 0;
}

static void __exit module_a_exit(void)
{
    kfree(ring);

    pr_info("Ring Removed\n");
}


module_init(module_a_init);
module_exit(module_a_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Threadsafe ring buffer Between Kernel Modules - Module A");

