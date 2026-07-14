#include <linux/module.h>
#include <linux/kernel.h>

extern char *get_shared_buffer(void);
extern size_t get_shared_buffer_size(void);

static int __init module_b_init(void)
{
    char *buf;

    buf = get_shared_buffer();

    if (!buf) {
        pr_err("Shared Buffer NULL\n");
        return -ENOMEM;
    }

    pr_info("Module B Loaded\n");

    pr_info("Buffer Address : %px\n", buf);

    pr_info("Original : %s\n", buf);

    snprintf(buf,
             get_shared_buffer_size(),
             "Modified by Module B");

    pr_info("Modified : %s\n", buf);

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
MODULE_DESCRIPTION("Zero Copy buffer sharing - Module B");