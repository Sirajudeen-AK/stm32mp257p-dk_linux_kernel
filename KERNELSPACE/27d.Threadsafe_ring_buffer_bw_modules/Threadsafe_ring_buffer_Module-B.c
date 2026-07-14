#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

extern int ring_push(const char *);
extern int ring_pop(char *);

static struct task_struct *consumer;

static int consumer_thread(void *data)
{
    char msg[64];

    while (!kthread_should_stop()) {

        if (!ring_pop(msg))
            pr_info("Consumer : %s\n", msg);
    }

    return 0;
}

static int __init module_b_init(void)
{
    consumer = kthread_run(
                    consumer_thread,
                    NULL,
                    "consumer");

    msleep(1000);

    ring_push("Packet A");

    msleep(1000);

    ring_push("Packet B");

    msleep(1000);

    ring_push("Packet C");

    return 0;
}

static void __exit module_b_exit(void)
{
    if (consumer)
        kthread_stop(consumer);
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Threadsafe ring buffer Between Kernel Modules - Module B");