/*
flow of execution:
		insmod
		↓
		probe()
		↓
		kthread_run()
		↓
		Kernel Thread Created
		↓
		Thread sleeps
		↓
		Button Press
		↓
		ISR
		↓
		data_ready = true
		↓
		wake_up_interruptible()
		↓
		Kernel Thread wakes
		↓
		Process Button
		↓
		Sleep again

*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>

struct my_device
{
    int gpio;
    int irq;

    struct task_struct *thread;

    wait_queue_head_t waitq;

    bool data_ready;
};

static int thread_fn(void *arg)
{
    struct my_device *mydev = arg;

    printk("Kernel thread started\n");

    while (!kthread_should_stop())
    {
        wait_event_interruptible(
                mydev->waitq,
                mydev->data_ready ||
                kthread_should_stop());

        if (kthread_should_stop())
            break;

        mydev->data_ready = false;

        printk("Thread Processing Button Event\n");

        msleep(2000);

        printk("Thread Finished\n");
    }

    printk("Kernel thread exiting\n");

    return 0;
}

static irqreturn_t button_irq(int irq, void *dev)
{
    struct my_device *mydev = dev;

    printk("ISR Executed\n");

    mydev->data_ready = true;

    wake_up_interruptible(&mydev->waitq);

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

    init_waitqueue_head(&mydev->waitq);

    mydev->gpio = of_get_named_gpio(
                    pdev->dev.of_node,
                    "button-gpio",
                    0);

    if (!gpio_is_valid(mydev->gpio))
        return -EINVAL;

    ret = devm_gpio_request_one(
            &pdev->dev,
            mydev->gpio,
            GPIOF_IN,
            "button");

    if (ret)
        return ret;

    mydev->irq = gpio_to_irq(mydev->gpio);

    ret = devm_request_irq(
            &pdev->dev,
            mydev->irq,
            button_irq,
            IRQF_TRIGGER_FALLING,
            "button_irq",
            mydev);

    if (ret)
        return ret;

    mydev->thread =
        kthread_run(thread_fn,
                    mydev,
                    "button_thread");

    if (IS_ERR(mydev->thread))
        return PTR_ERR(mydev->thread);

    platform_set_drvdata(pdev, mydev);

    printk("Driver Loaded\n");

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    struct my_device *mydev;

    mydev = platform_get_drvdata(pdev);

    if (mydev->thread)
        kthread_stop(mydev->thread);

    printk("Driver Removed\n");

    return 0;
}

static const struct of_device_id my_match[] = {
    {
        .compatible = "siraj,my-button",
    },
    {}
};

MODULE_DEVICE_TABLE(of, my_match);

static struct platform_driver my_driver = {

    .probe = my_probe,

    .remove = my_remove,

    .driver = {

        .name = "button",

        .of_match_table = my_match,
    },
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Kernel Thread and IRQ Example");	