#include <linux/module.h>
#include <linux/workqueue.h>

static struct delayed_work my_dwork;

static void delayed_work_fn(struct work_struct *work)
{
    pr_info("Delayed work executed\n");
}

static int __init my_init(void)
{
    INIT_DELAYED_WORK(&my_dwork, delayed_work_fn);

    pr_info("Scheduling work after 5 seconds\n");

    schedule_delayed_work(&my_dwork,
                          msecs_to_jiffies(5000));

    return 0;
}

static void __exit my_exit(void)
{
    cancel_delayed_work_sync(&my_dwork);

    pr_info("Module unloaded\n");
}

/* module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("A simple delayed workqueue example"); */