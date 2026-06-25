#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

static struct work_struct my_work;

static void work_handler(struct work_struct *work)
{
    int i;

    pr_info("Work started\n");

    for (i = 0; i < 10; i++) {
        pr_info("Working %d\n", i);
        msleep(1000);
    }

    pr_info("Work finished\n");
}

static int __init my_init(void)
{
	/*
			my_work
			|
			+--> work_handler()
	*/
    INIT_WORK(&my_work, work_handler);

	/*
		Queues work to kernel worker thread.
		Returns immediately.	
	*/
    schedule_work(&my_work);

    pr_info("Module loaded\n");

    return 0;
}

static void __exit my_exit(void)
{
	/*
		Wait until work finishes.
		Useful during module unload.
	*/
    flush_work(&my_work);

    pr_info("Module unloaded\n");
}

/* module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("A simple workqueue example"); */