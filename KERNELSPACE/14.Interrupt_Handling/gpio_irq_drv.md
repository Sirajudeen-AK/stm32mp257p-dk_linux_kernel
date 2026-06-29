# Program 14 — Interrupt Handling (GPIO IRQ)

## 1. Why Do We Need Interrupts?

Without interrupts the CPU must keep asking (polling), wasting time:

```c
CPU
while (1)
{
    Is UART data available?
    Is UART data available?
    Is UART data available?
}
```

With an interrupt, the hardware tells the CPU only when it needs attention:

```
CPU doing other work

        UART receives byte
               │
               ▼
        Interrupt generated
               │
               ▼
        CPU jumps to ISR
               │
               ▼
        Read hardware
               │
               ▼
        Return
```

The CPU is free until the hardware actually needs servicing.

---

## 2. Linux Interrupt Flow

```
Hardware
   │
IRQ Line
   │
GIC (Generic Interrupt Controller)
   │
Linux IRQ Subsystem
   │
request_irq()       ← your driver registers a handler here
   │
  ISR               ← your handler runs when the IRQ fires
```

### Registering the handler

```c
irq = gpio_to_irq(my_gpio);
request_irq(irq, my_isr, IRQF_TRIGGER_RISING, "my_button", dev);

static irqreturn_t my_isr(int irq, void *dev_id)
{
    // keep it SHORT — no sleeping
    return IRQ_HANDLED;   // or IRQ_NONE if this wasn't my device
}

free_irq(irq, dev);   // on cleanup
```

---

## 3. Interview Questions & Answers

**Q1. Why keep an ISR short?**

> While an interrupt is being serviced, interrupts are disabled or limited. A long
> ISR increases interrupt latency and delays handling of other devices, hurting
> system responsiveness.

**Q2. Why can't you sleep in an ISR?**

> An ISR runs in **atomic (interrupt) context** with no process backing, so it
> cannot be scheduled out. Calling a sleeping function triggers:
> `BUG: sleeping function called from invalid context`. Defer sleeping work to a
> workqueue or thread.

**Q3. Difference between interrupt and polling?**

| Polling               | Interrupt             |
| --------------------- | --------------------- |
| CPU checks repeatedly | Hardware notifies CPU |
| Wastes CPU            | Efficient             |
| Higher latency        | Lower latency         |

> Polling makes the CPU repeatedly check the device; interrupts let the hardware
> signal the CPU only when needed — more efficient and lower latency.

**Q4. Why would an ISR return `IRQ_NONE`?**

> On a **shared** IRQ line, each handler must check whether *its* device actually
> raised the interrupt. If not, it returns `IRQ_NONE` so the kernel calls the next
> registered handler. Return `IRQ_HANDLED` only when your device caused it.

**Q5. What is the top-half / bottom-half split?**

> The "top half" is the ISR — fast, atomic, acknowledges the hardware. The "bottom
> half" (softirq, tasklet, or workqueue) does the slower processing later. This
> keeps interrupts disabled for the minimum time.

**Q6. How do you get an IRQ number from a GPIO?**

> Use `gpio_to_irq(gpio)` (or fetch it from the Device Tree). The returned Linux
> IRQ number is then passed to `request_irq()`.

---

## 4. Debug Notes (STM32MP257)

**If the module fails to load because a resource is busy**, temporarily release the
conflicting `gpio-keys` driver:

```bash
echo gpio-keys > /sys/bus/platform/drivers/gpio-keys/unbind
insmod Ukernel_gpio_irq_drv.ko
dmesg -w
# ... test the button ...
rmmod Ukernel_gpio_irq_drv
echo gpio-keys > /sys/bus/platform/drivers/gpio-keys/bind
```

**Check who is currently using a pin** (find the pin name, e.g. `PC11`, from the
board schematic):

```bash
cat /sys/kernel/debug/gpio | grep -i PC11
```

**List the APIs/attributes a driver exposes:**

```bash
ls -l /sys/bus/platform/drivers/gpio-keys
```
