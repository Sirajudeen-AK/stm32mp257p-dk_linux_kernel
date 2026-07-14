#include <linux/module.h>
#include <linux/kernel.h>

extern void increment_counter(void);
extern int get_counter(void);

static int __init module_b_init(void)
{
    pr_info("Module B Loaded\n");

    increment_counter();
    increment_counter();
    increment_counter();

    pr_info("Counter = %d\n",
            get_counter());

    return 0;
}

static void __exit module_b_exit(void)
{
    pr_info("Module B Unloaded\n");
}

module_init(module_b_init);
module_exit(module_b_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Exporting APIs Between Kernel Modules - Module B");