#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *my_thread1, *my_thread2;

static int thread_function_1(void *data)
{
    int count = 0;

    pr_info("Kernel thread PID=%d started \n", current->pid);

	/*
		kthread_should_stop() is a function that checks if the thread should stop running. 
		It returns true if the thread has been requested to stop, and false otherwise. 
		The while loop continues to run as long as kthread_should_stop() returns false, 
		allowing the thread to perform its tasks until it is explicitly stopped.
	*/
    while (!kthread_should_stop()) {

        pr_info("Kernel thread PID=%d and running count=%d\n",
                current->pid, count++);

        msleep(1);
    }

    pr_info("Kernel thread PID=%d stopping\n", current->pid);

    return 0;
}

static int __init my_init(void)
{

	int target_cpu = 1; // Specify the target CPU for the second thread

	/*
	kthread_run() is a function that creates and starts a new kernel thread. ( kthread_create() + wake_up_process() )
	Arguments:
	1. thread_function: The function to be executed by the thread.
	2. NULL: The data to be passed to the thread function (not used in this case).
	3. "my_kthread": The name of the thread.
	*/
    my_thread1 = kthread_run(thread_function_1,
                            NULL,
                            "my_kthread-1");

    if (IS_ERR(my_thread1)) {
        pr_err("Failed to create thread-1\n");
        return PTR_ERR(my_thread1);
    }

	my_thread2 = kthread_create(thread_function_1,
                            NULL,
                            "my_kthread-2");

    if (!IS_ERR(my_thread2)) 
	{
		// Force the kthread to only execute on target_cpu-1
        kthread_bind(my_thread2, target_cpu);
        wake_up_process(my_thread2);
	} 
	else {
		pr_err("Failed to create thread-2\n");
		kthread_stop(my_thread1);
		return PTR_ERR(my_thread2);
    }

    pr_info("Module loaded and thread-1 PID=%d and thread-2 PID=%d\n", my_thread1->pid, my_thread2->pid);

    return 0;
}

static void __exit my_exit(void)
{
    kthread_stop(my_thread1);
    kthread_stop(my_thread2);

    pr_info("Module unloaded and thread-1 PID=%d and thread-2 PID=%d\n", my_thread1->pid, my_thread2->pid);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("A simple kernel thread example");