/*
Flow of Execution
		Button Press
			│
			▼
		GPIO Interrupt
			│
			▼
		ISR (Very Fast)
			│
		schedule_work()
			│
		Return Immediately
			│
			▼
		Kernel Worker Thread   <--> also called kworker or ksoftirqd or kthread or workqueue thread
			│
		Long Processing
		(msleep(), mutex, SPI, I2C...)

*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

struct my_device
{
    int gpio;
    int irq;

    struct work_struct work;
};

static void my_work_func(struct work_struct *work)
{
    struct my_device *mydev;

    mydev = container_of(work,
                         struct my_device,
                         work);

    printk("Workqueue started\n");

    msleep(3000);

    printk("GPIO=%d Processing Finished\n",
            mydev->gpio);
}

static irqreturn_t button_irq(int irq, void *dev)
{
    struct my_device *mydev = dev;

    printk("ISR Executed\n");

    schedule_work(&mydev->work);

    return IRQ_HANDLED;
}

static int my_probe(struct platform_device *pdev)
{
    struct my_device *mydev;
    int ret;

    printk("Probe called\n");

    mydev = devm_kzalloc(&pdev->dev,
                         sizeof(*mydev),
                         GFP_KERNEL);

    if (!mydev)
        return -ENOMEM;

    INIT_WORK(&mydev->work,
              my_work_func);

    mydev->gpio =
        of_get_named_gpio(pdev->dev.of_node,
                          "button-gpio",
                          0);

    if (!gpio_is_valid(mydev->gpio))
        return -EINVAL;

    ret = devm_gpio_request_one(&pdev->dev,
                                mydev->gpio,
                                GPIOF_IN,
                                "button");

    if (ret)
        return ret;

    mydev->irq = gpio_to_irq(mydev->gpio);

    ret = devm_request_irq(&pdev->dev,
                           mydev->irq,
                           button_irq,
                           IRQF_TRIGGER_FALLING,
                           "button_irq",
                           mydev);

    if (ret)
        return ret;

    platform_set_drvdata(pdev, mydev);

    printk("Driver Loaded\n");

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    struct my_device *mydev;

    mydev = platform_get_drvdata(pdev);

    cancel_work_sync(&mydev->work);

    printk("Driver Removed\n");

    return 0;
}

static const struct of_device_id my_match[] = {
    {
        .compatible = "siraj,my-button",
    },
    { }
};

MODULE_DEVICE_TABLE(of, my_match);

static struct platform_driver my_driver = {

    .probe = my_probe,

    .remove = my_remove,

    .driver = {

        .name = "my_button",

        .of_match_table = my_match,
    },
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Workqueue (HW ISR -> Deferred Processing)");