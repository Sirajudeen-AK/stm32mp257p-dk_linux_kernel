#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

static struct timer_list my_timer;

static void timer_callback(struct timer_list *t)
{
    pr_info("Timer expired! jiffies=%lu\n", jiffies);
}

static int __init my_init(void)
{
    pr_info("Module loaded\n");

	/*
		API - timer_setup() is used to initialize the timer_list structure.
		Arguments:
			1. &my_timer: Pointer to the timer_list structure.
			2. timer_callback: The callback function that will be called when the timer expires.
			3. 0: Flags (not used in this case).
	*/
    timer_setup(&my_timer, timer_callback, 0);

	/*
		API - mod_timer() is used to start the timer.
		Arguments:
			1. &my_timer: Pointer to the timer_list structure.
			2. jiffies + msecs_to_jiffies(5000): The expiration time in jiffies. 
			Here, we are setting the timer to expire after 5000 milliseconds
	*/
    mod_timer(&my_timer,
              jiffies + msecs_to_jiffies(5000));

    pr_info("Timer started for 5 seconds\n");

    return 0;
}

static void __exit my_exit(void)
{
	/*
		API - del_timer_sync() is used to delete the timer and ensure that the timer callback function is not running 
		when the module is unloaded.
		Arguments:
			1. &my_timer: Pointer to the timer_list structure.

		Note: It waits if the callback is currently executing, preventing use-after-free bugs during module unload.
	*/
    del_timer_sync(&my_timer);

    pr_info("Module unloaded\n");
}

/* module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("A simple kernel timer example"); */