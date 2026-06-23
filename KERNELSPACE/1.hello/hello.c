#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
	/* pr_info -> to store kernel log buffer using dmesg will verify */
    pr_info("Hello STM32 Kernel\n");
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("Goodbye STM32 Kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("First STM32 Module");