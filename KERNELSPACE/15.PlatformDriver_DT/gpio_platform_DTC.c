/* 
driver executes in this order:
		insmod
		│
		▼
		module_platform_driver()
		│
		▼
		platform_driver_register()
		│
		▼
		Kernel searches Device Tree
		│
		▼
		compatible = "siraj,my-button"
		│
		▼
		Match found
		│
		▼
		probe()
		│
		▼
		Read button-gpio property
		│
		▼
		GPIO = 656
		│
		▼
		gpio_to_irq()
		│
		▼
		IRQ = 88
		│
		▼
		request_irq()
		│
		▼
		Wait for hardware interrupt

Then, when you press the button:

		Button
		│
		▼
		GPIO 656 changes state
		│
		▼
		STM32 GPIO controller
		│
		▼
		IRQ 88
		│
		▼
		GIC (ARM Interrupt Controller)
		│
		▼
		Linux IRQ subsystem
		│
		▼
		button_irq()
		│
		▼
		printk("Button Pressed")

Finally:
		rmmod
		│
		▼
		remove()
*/
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

struct my_device {

    int gpio;
    int irq;
};

static unsigned long last_jiffies;

static irqreturn_t button_irq(int irq, void *dev)
{
	/*
		timer_after is a macro that checks if the current time (jiffies) is after the last time 
		we handled an interrupt plus half a second (HZ / 2).
	*/
    if (time_after(jiffies, last_jiffies + HZ / 2)) {
        printk("Button Pressed and jiffies=%lu\n", jiffies);
        last_jiffies = jiffies;
    }

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

    ret = devm_request_irq(
            &pdev->dev,
            mydev->irq,
            button_irq,
            IRQF_TRIGGER_FALLING, /* dts configuration is GPIO_ACTIVE_LOW */
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
MODULE_DESCRIPTION("Platform Driver with Device Tree");