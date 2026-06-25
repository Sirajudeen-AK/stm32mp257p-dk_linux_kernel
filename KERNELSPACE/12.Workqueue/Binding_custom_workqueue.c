#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/atomic.h> // 1. Added for atomic operations

static struct workqueue_struct *my_wq;
static struct work_struct my_work;

// 2. Define a stop flag (0 = run, 1 = stop)
static atomic_t stop_work_flag = ATOMIC_INIT(0);

static void work_handler(struct work_struct *work)
{
    int i;

    pr_info("Work started on CPU %d\n", smp_processor_id());

	/* 
		If we use infinite loop, we need a way to exit gracefully.
		We can use an atomic flag to signal the work handler to stop.
		Otherwise, the module unload will hang indefinitely.
	*/
    while(1) {
        // 3. Check if the exit function raised the stop flag
        if (atomic_read(&stop_work_flag) == 1) {
            pr_info("Work handler received stop signal. Aborting loop early at i=%d\n", i);
            break; 
        }

        pr_info("Working %d on CPU %d\n", i, smp_processor_id());
        msleep(10);
    }

    pr_info("Work finished\n");
}

static int __init my_init(void)
{
    int target_cpu = 1; 

    // 4. Ensure flag is cleared on startup
    atomic_set(&stop_work_flag, 0);

	/* 
		Allocate a dedicated custom workqueue
	*/
    my_wq = create_workqueue("my_custom_wq");
    if (!my_wq) {
        pr_err("Failed to create workqueue\n");
        return -ENOMEM;
    }

    INIT_WORK(&my_work, work_handler);

	/*
        Queues work to a specific CPU core's worker thread.
        Returns immediately.     
    */
    queue_work_on(target_cpu, my_wq, &my_work);

    pr_info("Module loaded. Work bound to CPU %d\n", target_cpu);

    return 0;
}

static void __exit my_exit(void)
{
    pr_info("Unloading module... Signaling work handler to stop.\n");

    // 5. Raise the stop flag first so the loop breaks early
    atomic_set(&stop_work_flag, 1);

    /*
        Wait until work finishes.
        Since the loop breaks early, this will return almost instantly.
    */
    flush_work(&my_work);

    /* Clean up the allocated workqueue infrastructure */
    if (my_wq) {
        destroy_workqueue(my_wq);
    }

    pr_info("Module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("A simple workqueue example with CPU binding");