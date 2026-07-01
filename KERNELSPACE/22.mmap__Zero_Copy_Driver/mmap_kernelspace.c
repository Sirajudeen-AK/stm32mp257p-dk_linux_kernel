
/*

		open()
		↓
		mmap()
		↓
		Kernel maps page
		↓
		User gets pointer
		↓
		printf(ptr)
		↓
		Hello from Kernel mmap!
		↓
		strcpy(ptr,"Hello Userspace")
		↓
		Kernel buffer changes immediately

*/


#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mmap_demo"

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static char *page;

static int my_open(struct inode *inode, struct file *file)
{
    printk("opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    printk("closed\n");
    return 0;
}

static int my_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long size;

	sprintf(page, "Hello from Kernel and Kernal Virtual - 0x%px and Physical Address - 0x%pa\n", page, &(virt_to_phys(page)));

    size = vma->vm_end - vma->vm_start;

	/* PAGE_SIZE is 4096 bytes on most architectures */
    if (size > PAGE_SIZE)
        return -EINVAL;

	/* Check if the offset is non-zero */
    if (vma->vm_pgoff != 0)
        return -EINVAL;

	/* 
		API - remap_pfn_range
			used to map a physical page frame into the user-space virtual address space. 
			It allows the kernel to provide direct access to a specific physical page, 
			enabling zero-copy data sharing between kernel and user space.
		Arguments:
			vma: A pointer to the vm_area_struct structure representing the user-space virtual memory area to be mapped.
			vma->vm_start: The starting virtual address of the user-space memory area to be mapped.
			virt_to_phys(page) >> PAGE_SHIFT: The physical page frame number corresponding to the kernel page to be mapped. 
				The virt_to_phys function converts the virtual address of the kernel page to its physical address, 
				and PAGE_SHIFT is used to obtain the page frame number.
			size: The size of the memory area to be mapped (in bytes).
			vma->vm_page_prot: The page protection flags for the mapped memory area,
				which specify the access permissions (read, write, execute) for the user-space memory area.

				
				Kernel Page
					▲
					│
				Same Physical Memory
					│
				Userspace Pointer

				There is no copy_to_user() and no copy_from_user().
				Both kernel and userspace access the same physical page.
		
	*/
    if (remap_pfn_range(vma,
                        vma->vm_start,
                        virt_to_phys(page) >> PAGE_SHIFT,
                        size,
                        vma->vm_page_prot))
        return -EAGAIN;

    printk("mmap successful\n");

    return 0;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .mmap = my_mmap,
};

static int __init my_init(void)
{
    int ret;

    page = (char *)get_zeroed_page(GFP_KERNEL);
    if (!page)
        return -ENOMEM;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        goto err_page;

    cdev_init(&my_cdev, &fops);

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret)
        goto err_chrdev;

    my_class = class_create(DEVICE_NAME);
    if (IS_ERR(my_class))
    {
        ret = PTR_ERR(my_class);
        goto err_cdev;
    }

    my_device = device_create(my_class,
                              NULL,
                              dev_num,
                              NULL,
                              DEVICE_NAME);

    if (IS_ERR(my_device))
    {
        ret = PTR_ERR(my_device);
        goto err_class;
    }

    printk("Major=%d Minor=%d\n",
           MAJOR(dev_num),
           MINOR(dev_num));

    return 0;

err_class:
    class_destroy(my_class);

err_cdev:
    cdev_del(&my_cdev);

err_chrdev:
    unregister_chrdev_region(dev_num,1);

err_page:
    free_page((unsigned long)page);

    return ret;
}

static void __exit my_exit(void)
{
    device_destroy(my_class, dev_num);

    class_destroy(my_class);

    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num,1);

    free_page((unsigned long)page);

    printk("removed\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Simple mmap demo driver");