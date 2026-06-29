#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>

#define BUF_SIZE 4096

struct my_device {

    void *cpu_addr;

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

	/*
		Allocate a DMA buffer of size BUF_SIZE and get the CPU and bus addresses for the device.
	*/
    mydev->cpu_addr =
        dma_alloc_coherent(
                &pdev->dev,
                BUF_SIZE,
                &mydev->dma_addr,
                GFP_KERNEL);

    if (!mydev->cpu_addr)
        return -ENOMEM;

    memset(mydev->cpu_addr,0,BUF_SIZE);

    printk("DMA CPU Addr : %p\n",
            mydev->cpu_addr);

    printk("DMA Bus Addr : %pad\n",
            &mydev->dma_addr);

    platform_set_drvdata(pdev,mydev);

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    struct my_device *mydev;

    mydev = platform_get_drvdata(pdev);

	/*
		Free the previously allocated DMA buffer.
	*/
    dma_free_coherent(
            &pdev->dev,
            BUF_SIZE,
            mydev->cpu_addr,
            mydev->dma_addr);

    printk("DMA Freed\n");

    return 0;
}

static const struct of_device_id my_match[] = {

    {
        .compatible="siraj,my-button",
    },

    {}
};

MODULE_DEVICE_TABLE(of,my_match);

static struct platform_driver my_driver = {

	.probe=my_probe,

	.remove=my_remove,

	.driver={

		.name="dma_demo",

		.of_match_table=my_match,

	},
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("DMA Driver");