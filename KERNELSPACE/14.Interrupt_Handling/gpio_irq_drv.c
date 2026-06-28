
/* 
Goal
		Push Button
			│
			▼
		GPIO Pin
			│
			▼
		GPIO Interrupt
			│
			▼
		Linux ISR
			│
			▼
		printk()

Later we'll extend this to:

		Button
		↓
		ISR
		↓
		Workqueue
		↓
		Character Driver
		↓
		poll()/read()

which is a real driver architecture.

*/



#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

/*
 * GPIO number is runtime-configurable so you can try different header pins
 * without recompiling this module.
 *
 * Default: 555 (PC11 / USER2 button).


| Thing            | Value        |
| ---------------  | ----------   |
| Header pin       | BUTTON.USER2 |  -> check schematics for your board
| STM32 pin        | PC11          |  -> backtrace from header pin to STM32 pin
| Linux GPIO chip  | gpiochip2    |  -> check "cat /sys/kernel/debug/gpio"
| GPIO number      | 555          |	 -> check "cat /sys/kernel/debug/gpio"

 */

static unsigned int gpio_num = 555;
module_param(gpio_num, uint, 0644);
MODULE_PARM_DESC(gpio_num, "Linux GPIO number for IRQ input (default: 555 = USER2/PC11)");

static int irq_number;
static int pressed = 0;

static DECLARE_WAIT_QUEUE_HEAD(wq);

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    printk("GPIO IRQ triggered!\n");
    pressed = 1;
    wake_up_interruptible(&wq);
    return IRQ_HANDLED;
}

static ssize_t dev_read(struct file *file, char __user *buf,
                        size_t len, loff_t *off)
{
    wait_event_interruptible(wq, pressed == 1);

    pressed = 0;

    if (copy_to_user(buf, "BUTTON_PRESSED\n", 16))
        return -EFAULT;

    return 16;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
};

static int __init gpio_init(void)
{
    int ret;

    printk("GPIO IRQ driver loading...\n");

	/*
	API - gpio_request 
		used to request control of a specific GPIO pin. 
		It ensures that the specified GPIO pin is available for use by the driver, 
		and it prevents other drivers from using the same GPIO pin simultaneously.

	Arguments:
		gpio_num: The GPIO number to request.
		"gpio_irq_input": A label or name for the GPIO pin being requested.
	*/
	ret = gpio_request(gpio_num, "gpio_irq_input");
    if (ret) {
		if (ret == -EBUSY)
			printk("GPIO %u is busy (often owned by gpio-keys or another driver)\n", gpio_num);
		else
			printk("GPIO request failed for %u (ret=%d)\n", gpio_num, ret);
        return ret;
    }

	/*
	API - gpio_direction_input
		used to configure the specified GPIO pin as an input. 
		It sets the direction of the GPIO pin to input mode, allowing the driver to read the state of the pin.
	Arguments:
		gpio_num: The GPIO number to configure as input.
	*/
	gpio_direction_input(gpio_num);

	/*
	API - gpio_to_irq
		used to map a GPIO number to an IRQ number. 
		It allows the driver to obtain the IRQ number associated with a specific GPIO pin, 
		enabling the driver to request and handle interrupts for that GPIO pin.
	Arguments:
		gpio_num: The GPIO number to be mapped to an IRQ.
	*/
	irq_number = gpio_to_irq(gpio_num);

	/*
		API - request_irq
			used to request an interrupt line and register an interrupt handler for a specific GPIO pin. 
			It associates the specified IRQ number with the provided interrupt handler function, 
			allowing the kernel to invoke the handler when the corresponding interrupt occurs.

		Arguments:
			irq_number: The IRQ number associated with the GPIO pin.
			gpio_irq_handler: The interrupt handler function to be called when the interrupt occurs.
			IRQF_TRIGGER_RISING: For USER2 (PC11 / GPIO 555), button press is active-high,
								 so IRQ should trigger on rising edge (press event).
				if the button were active-low, we would use IRQF_TRIGGER_FALLING instead.
			"gpio_irq_drv": The name of the device requesting the IRQ.
			NULL: A pointer to the device-specific data (not used in this example).
	*/
    ret = request_irq(irq_number,
                      gpio_irq_handler,
                      IRQF_TRIGGER_RISING,
                      "gpio_irq_drv",
                      NULL);

    if (ret) {
        printk("IRQ request failed\n");
		gpio_free(gpio_num);
        return ret;
    }

	/*
	API - register_chrdev
		used to register a character device with the kernel. 
		It associates a major number with the specified device name and file operations, 
		enabling user-space applications to interact with the device through standard file I/O operations.
	Arguments:
		240: The major number for the character device.
		"gpio_irq_drv": The name of the character device.
		&fops: A pointer to the file_operations structure that defines the operations for the character device.
	
	*/
    register_chrdev(240, "gpio_irq_drv", &fops);

	printk("Driver loaded: GPIO %u IRQ %d\n", gpio_num, irq_number);
    return 0;
}

static void __exit gpio_exit(void)
{
    free_irq(irq_number, NULL);
	gpio_free(gpio_num);
    unregister_chrdev(240, "gpio_irq_drv");

    printk("Driver unloaded\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("GPIO Interrupt Handling Example");