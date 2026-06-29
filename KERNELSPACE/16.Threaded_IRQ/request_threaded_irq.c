
/*
Flow of execution of a threaded interrupt handler
		User presses button
			│
			▼
		Hardware IRQ
			│
			▼
		Top Half
			│
		printk()
		Save flags
			│
		IRQ_WAKE_THREAD
			│
		Top Half exits immediately   <-- IRQ exists once the thread is woken up
			│
		CPU free again
			│
		Kernel wakes IRQ thread
			│
		Bottom Half starts
			│
		msleep()
		mutex_lock()
		SPI
		I2C
		copy data
			│
		 Done

*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

struct my_device {

    int gpio;
    int irq;
};

static irqreturn_t top_irq(int irq, void *dev)
{
    printk("Top Half\n");

	/* IRQ_WAKE_THREAD indicates that the top half handler has done minimal work and 
		the bottom half should be executed in a separate thread */
    return IRQ_WAKE_THREAD;
}

/*
	thread_irq() is the bottom half interrupt handler function that will be executed in a separate kernel 
	thread context. It is called when the top half handler returns IRQ_WAKE_THREAD.
*/
static irqreturn_t thread_irq(int irq, void *dev)
{
    printk("Thread started\n");

    msleep(1000);

    printk("Thread finished\n");

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

    mydev->gpio = of_get_named_gpio(
                    pdev->dev.of_node,
                    "button-gpio",   /* I modified this in /arch/arm64/boot/dts/st/stm32mp257f-dk.dts */
                    0);

    if (!gpio_is_valid(mydev->gpio))
    {
        printk("Invalid GPIO\n");
        return -EINVAL;
    }

    ret = devm_gpio_request_one(
                &pdev->dev,
                mydev->gpio,
                GPIOF_IN,
                "button");

    if (ret)
        return ret;

    mydev->irq = gpio_to_irq(mydev->gpio);

	/*
		API request_threaded_irq()
			Parameters:
				irq: The interrupt number to be requested
				top_irq: The top half interrupt handler function
				thread_irq: The bottom half interrupt handler function
				flags: Interrupt flags (e.g., IRQF_TRIGGER_FALLING)
				name: Name of the interrupt
				dev: Pointer to device-specific data

			Return value:
				On success, returns 0. On failure, returns a negative error code.
	*/
	ret = devm_request_threaded_irq(
            &pdev->dev,
            mydev->irq,
            top_irq,
            thread_irq,
            IRQF_TRIGGER_FALLING,   /* dts configuration is GPIO_ACTIVE_LOW */
            "button_irq",
            mydev);

    if (ret)
        return ret;

    platform_set_drvdata(pdev, mydev);

    printk("GPIO=%d IRQ=%d\n",
            mydev->gpio,
            mydev->irq);

    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    printk("Remove called\n");
    return 0;
}


/*
	Device Tree match table
*/
static const struct of_device_id my_match[] = {

    {
        .compatible = "siraj,my-button",
    },

    { }
};

/* 
	Register the device table 
	And add the MODULE_DEVICE_TABLE macro to make sure the module is loaded when the device is present
*/
MODULE_DEVICE_TABLE(of, my_match);

/* Platform driver structure */
static struct platform_driver my_driver = {

    .probe = my_probe,

    .remove = my_remove,

    .driver = {

        .name = "my_button",  /* I modified this in /arch/arm64/boot/dts/st/stm32mp257f-dk.dts */

        .of_match_table = my_match,
    },
};

/* 
	Register the platform driver 
	create the module init and exit functions
	register the platform driver add the module information
*/
module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("A simple platform driver for a button with threaded interrupt handling");
