/*
The DMA buffer becomes our driver's data buffer.

Userspace
		echo "Hello"
			│
			write()
			│
		copy_from_user()
			│
		DMA Buffer
			│
			read()
			│
		copy_to_user()
			│
		cat /dev/mychar


Driver Architecture

				probe()
                  │
        dma_alloc_coherent()
                  │
          DMA CPU Address
                  │
           register cdev
                  │
     		  /dev/mychar
      		  /        \
    	   write()    read()
    	     │         │
 	copy_from_user  copy_to_user
       		  │         │
  		       DMA Buffer

*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/err.h>

#define DEVICE_NAME "mychar_dma"
#define BUF_SIZE 4096

struct my_device
{
    dev_t dev;

    struct cdev cdev;

    struct class *class;

    struct device *device;

    void *cpu_addr;

    dma_addr_t dma_addr;

    size_t size;
};

static int my_open(struct inode *inode,
                   struct file *file)
{
    struct my_device *mydev;

    mydev = container_of(inode->i_cdev,
                         struct my_device,
                         cdev);

    file->private_data = mydev;

    printk("opened\n");

    return 0;
}

static int my_release(struct inode *inode,
                      struct file *file)
{
    printk("closed\n");
    return 0;
}

static ssize_t my_write(struct file *file,
                        const char __user *buf,
                        size_t len,
                        loff_t *off)
{
    struct my_device *mydev = file->private_data;

    if (len > BUF_SIZE)
        len = BUF_SIZE;

    if (copy_from_user(mydev->cpu_addr,
                       buf,
                       len))
        return -EFAULT;

    mydev->size = len;

    printk("DMA Buffer Updated\n");

    return len;
}

static ssize_t my_read(struct file *file,
                       char __user *buf,
                       size_t len,
                       loff_t *off)
{
    struct my_device *mydev = file->private_data;

    if (*off >= mydev->size)
        return 0;

    if (len > mydev->size - *off)
        len = mydev->size - *off;

    if (copy_to_user(buf,
                     mydev->cpu_addr + *off,
                     len))
        return -EFAULT;

    *off += len;

    return len;
}

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.read = my_read,
	.write = my_write,
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *mydev;
    int ret;

    mydev = devm_kzalloc(&pdev->dev,
                         sizeof(*mydev),
                         GFP_KERNEL);

    if (!mydev)
        return -ENOMEM;

    mydev->cpu_addr =
        dma_alloc_coherent(&pdev->dev,
                           BUF_SIZE,
                           &mydev->dma_addr,
                           GFP_KERNEL);

    if (!mydev->cpu_addr)
        return -ENOMEM;

    ret = alloc_chrdev_region(&mydev->dev,
                              0,
                              1,
                              DEVICE_NAME);

    if (ret)
        goto err_dma;

    cdev_init(&mydev->cdev,
              &fops);

    mydev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&mydev->cdev,
                   mydev->dev,
                   1);

    if (ret)
        goto err_chr;

    mydev->class =
        class_create(DEVICE_NAME);

    if (IS_ERR(mydev->class)) {
        ret = PTR_ERR(mydev->class);
        goto err_cdev;
    }

    mydev->device =
        device_create(mydev->class,
                      NULL,
                      mydev->dev,
                      NULL,
                      DEVICE_NAME);

    if (IS_ERR(mydev->device)) {
        ret = PTR_ERR(mydev->device);
        goto err_class;
    }

    platform_set_drvdata(pdev,
                         mydev);

    printk("DMA CPU=%p\n",
            mydev->cpu_addr);

    printk("DMA BUS=%pad\n",
            &mydev->dma_addr);

    printk("major=%d\n",
            MAJOR(mydev->dev));

    printk("probe done: /dev/%s created\n",
            DEVICE_NAME);

    return 0;

err_class:

    class_destroy(mydev->class);

err_cdev:

    cdev_del(&mydev->cdev);

err_chr:

    unregister_chrdev_region(mydev->dev,
                             1);

err_dma:

    dma_free_coherent(&pdev->dev,
                      BUF_SIZE,
                      mydev->cpu_addr,
                      mydev->dma_addr);

    return ret;
}

static int my_remove(struct platform_device *pdev)
{
    struct my_device *mydev =
        platform_get_drvdata(pdev);

    if (mydev->class)
        device_destroy(mydev->class,
                       mydev->dev);

    if (mydev->class)
        class_destroy(mydev->class);

    cdev_del(&mydev->cdev);

    unregister_chrdev_region(mydev->dev,
                             1);

    dma_free_coherent(&pdev->dev,
                      BUF_SIZE,
                      mydev->cpu_addr,
                      mydev->dma_addr);

    printk("remove done: /dev/%s destroyed\n",
            DEVICE_NAME);

    return 0;
}

static const struct of_device_id my_ids[] =
{
    {
		.compatible="siraj,my-button"
	},
	
    {}
};

MODULE_DEVICE_TABLE(of,my_ids);

static struct platform_driver my_driver=
{
	.probe=my_probe,

	.remove=my_remove,

	.driver={
		.name="dma_char",

		.of_match_table=my_ids,
	},
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("siraj");
MODULE_DESCRIPTION("Character Driver using DMA Buffer");