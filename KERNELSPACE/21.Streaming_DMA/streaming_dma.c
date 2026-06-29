/*
Execution Flow of Streaming DMA Demo
	probe()
	↓
	kmalloc()
	↓
	CPU fills buffer
	↓
	dma_map_single()
	↓
	DMA Address
	↓
	(Hardware DMA happens here)
	↓
	dma_unmap_single()
	↓
	Remove
	↓
	kfree()

*/


#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/of.h>

#define BUF_SIZE 4096

struct my_device
{
    void *buffer;

    dma_addr_t dma_addr;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_device *mydev;

    mydev = devm_kzalloc(&pdev->dev,
                         sizeof(*mydev),
                         GFP_KERNEL);

    if (!mydev)
        return -ENOMEM;

    mydev->buffer = kmalloc(BUF_SIZE,
                            GFP_KERNEL);

    if (!mydev->buffer)
        return -ENOMEM;

    strcpy(mydev->buffer,
           "Hello DMA Mapping");

    mydev->dma_addr =
        dma_map_single(&pdev->dev,
                       mydev->buffer,
                       BUF_SIZE,
                       DMA_TO_DEVICE);

    if (dma_mapping_error(&pdev->dev,
                          mydev->dma_addr))
    {
        kfree(mydev->buffer);
        return -EIO;
    }

    printk("CPU Buffer : %p\n",
            mydev->buffer);

    printk("DMA Address : %pad\n",
            &mydev->dma_addr);

    /*
     * Normally hardware DMA would happen here.
     */

    dma_unmap_single(&pdev->dev,
                     mydev->dma_addr,
                     BUF_SIZE,
                     DMA_TO_DEVICE);

    platform_set_drvdata(pdev,
                         mydev);

    printk("Streaming DMA Demo Loaded\n");

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    struct my_device *mydev;

    mydev = platform_get_drvdata(pdev);

    kfree(mydev->buffer);

    printk("Driver Removed\n");

    return 0;
}

static const struct of_device_id my_match[] =
{
    {
        .compatible = "siraj,my-button",
    },
    {}
};

MODULE_DEVICE_TABLE(of, my_match);

static struct platform_driver my_driver =
{
    .probe = my_probe,

    .remove = my_remove,

    .driver =
    {
        .name = "stream_dma",

        .of_match_table = my_match,
    },
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Streaming DMA Demo");