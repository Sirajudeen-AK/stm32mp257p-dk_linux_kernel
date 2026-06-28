#include <linux/atomic.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

static struct timer_list my_timer;

static unsigned int interval_ms = 1000;
/* can be changed at module load time */
module_param(interval_ms, uint, 0644);
MODULE_PARM_DESC(interval_ms, "Periodic timer interval in milliseconds");

/*
 * stop_flag == 1 means module is unloading; callback must not re-arm timer.
 */
static atomic_t stop_flag = ATOMIC_INIT(0);

static void timer_callback(struct timer_list *t)
{
	pr_info("[periodic] Timer tick: jiffies=%lu\n", jiffies);

	if (atomic_read(&stop_flag))
		return;

	mod_timer(t, jiffies + msecs_to_jiffies(interval_ms));
}

static int __init my_init(void)
{
	pr_info("[periodic] Module loaded\n");
	atomic_set(&stop_flag, 0);

	timer_setup(&my_timer, timer_callback, 0);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(interval_ms));

	pr_info("[periodic] Timer started, interval=%u ms\n", interval_ms);
	return 0;
}

static void __exit my_exit(void)
{
	/* Prevent callback from re-arming while unload is in progress. */
	atomic_set(&stop_flag, 1);

	/* Waits for running callback and removes pending timer safely. */
	del_timer_sync(&my_timer);

	pr_info("[periodic] Module unloaded safely\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Periodic kernel timer with safe shutdown");
