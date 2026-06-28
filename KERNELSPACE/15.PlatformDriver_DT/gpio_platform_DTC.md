# Program 15 \u2014 Platform Driver + Device Tree

## 1. Why Do We Need a Platform Driver?

Until now drivers contained **hardcoded** values:

```c
#define GPIO_NUM 656
register_chrdev(...);
```

The driver "knew" everything about the hardware. But the same driver (LED, button,
CAN, UART, SPI, I2C sensor) should work on **different boards** where the hardware
differs:

```
Board A: LED -> GPIO 12
Board B: LED -> GPIO 90
Board C: LED -> GPIO 656
```

We must **not** edit the driver for every board. The board-specific information
belongs in the **Device Tree (DT)**, not in the driver code.

---

## 2. Architecture

Instead of hardcoding:

```
Driver
   |
Hardcoded GPIO          \u274c board-specific driver
```

we use:

```
Device Tree
        |
        v
Platform Bus
        |
        v
Platform Driver         \u2705 generic, board-independent driver
```

The driver becomes **generic**; the hardware details live in the DT.

---

## 3. How Linux Boots and Calls probe()

```
Bootloader
      |
Loads DTB (compiled Device Tree)
      |
Kernel parses DTB
      |
Creates platform devices
      |
Matches the "compatible" string
      |
Calls probe()
```

This is why `probe()` exists \u2014 it runs when a device is matched, not just when the
module loads.

---

## 4. Matching: compatible string

**Device Tree node (.dts):**

```dts
my_button {
    compatible = "siraj,my-button";
};
```

**Driver:**

```c
static const struct of_device_id my_of_match[] = {
    { .compatible = "siraj,my-button" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);
```

When Linux finds a DT node whose `compatible` matches the driver's table, it calls
`probe()` automatically.

---

## 5. Driver Lifecycle

```
insmod
    \u2193
platform_driver_register()   (driver registered)
    \u2193
kernel searches Device Tree
    \u2193
compatible matched
    \u2193
probe()                      (hardware initialized)
    \u2193
driver running

rmmod
    \u2193
remove()                     (cleanup)
```

If three identical devices exist in the DT, Linux calls `probe()` **three times** \u2014
one generic driver serves many devices.

---

## 6. Why probe() Instead of module_init()?

`module_init()` runs exactly **once** and has no idea which hardware exists.
A platform driver flows like this:

```
module_init()
    \u2193
platform_driver_register()
    \u2193
probe()              \u2190 called per matched device
    \u2193
Hardware initialized
```

So actual hardware setup happens in `probe()`, once per device, driven by the DT.

---

## 7. Where Does `pdev` Come From?

You never created a `platform_device`, yet `probe()` receives one:

```c
static int my_probe(struct platform_device *pdev)
```

**Linux creates it automatically** from the Device Tree at boot:

```
Bootloader
      │
      ▼
Loads DTB
      │
      ▼
Kernel parses Device Tree
      │
      ▼
Creates platform_device      ← you never allocate this; Linux does
      │
      ▼
Registers it on the platform bus
      │
      ▼
Driver registers
      │
      ▼
compatible matches
      │
      ▼
probe(struct platform_device *pdev)
```

---

## 8. What Is Inside `platform_device`?

Simplified view:

```c
struct platform_device {
    struct device   dev;        // the generic device
    const char     *name;
    int             id;
    struct resource *resource;  // memory ranges, IRQs, etc.
};
```

Everything is reached **through `pdev->dev`**, which is why we wrote
`pdev->dev.of_node`:

```
platform_device
        │
        ▼
      device        (pdev->dev)
        │
        ▼
     of_node        (pdev->dev.of_node)  ← the matched Device Tree node
```

---

## 9. Saving & Retrieving Your Private Data

Store your device structure during `probe()`, get it back in `remove()`:

```c
platform_set_drvdata(pdev, mydev);          // in probe()
struct my_device *mydev = platform_get_drvdata(pdev);  // in remove()
```

```
probe()                              remove()
   │                                    │
   ▼                                    ▼
platform_set_drvdata(pdev, mydev)   platform_get_drvdata(pdev) → mydev
```

This is the platform-driver equivalent of `file->private_data`:

| `file->private_data` | `platform_get_drvdata()` |
| -------------------- | ------------------------ |
| per **open()**       | per **device**           |

> Interviewers love this comparison.

---

## 10. Multiple Devices, One Driver

Two buttons in the Device Tree:

```dts
button1 { compatible = "siraj,my-button"; ... };
button2 { compatible = "siraj,my-button"; ... };
```

Linux creates two devices and calls `probe()` for each:

```
Device Tree
   │
   ├── button1 ──► platform_device #1 ──► probe(button1)
   │
   └── button2 ──► platform_device #2 ──► probe(button2)

                One driver  ──►  Many devices
```

This is one of the biggest advantages of the platform framework.

---

## 11. What If `compatible` Doesn't Match?

```
DTS:    compatible = "abc,mybutton";
Driver: compatible = "siraj,my-button";
              │
              ▼
        strings differ
              │
              ▼
   module loads, but probe() is NEVER called
```

A very common interview trap \u2014 the module loads successfully, yet nothing happens
because no device matched.

---

## 12. Device-Managed Resources (devm_*)

We used `devm_gpio_request_one()` instead of `gpio_request()`. The `devm_*` APIs
are **device-managed**: Linux automatically releases the GPIO, IRQ, and memory when
`remove()` runs.

```
probe()
   │
   ▼
devm_gpio_request_one()   ← resource tracked by the device
   │
   ▼
... driver runs ...
   │
   ▼
remove()  ──► Linux auto-frees GPIO / IRQ / memory
```

That's why we did **not** manually call `gpio_free()`, `free_irq()`, or `kfree()`.
Real kernel drivers prefer `devm_*` because it simplifies cleanup and prevents
resource-leak bugs.

---

## 13. Commands / Testing

```bash
sudo insmod gpio_platform_DTC.ko
dmesg | tail            # see probe() messages (or "no matching device")
ls /sys/bus/platform/drivers/    # confirm the driver registered
sudo rmmod gpio_platform_DTC
```

---

## 14. Interview Questions & Answers

**Q1. What is the difference between a platform device and a platform driver?**

> A **platform device** describes the hardware instance (created from the Device
> Tree). A **platform driver** is the code that handles it. The platform bus matches
> them via the `compatible` string and then calls the driver's `probe()`.

**Q2. Who creates the `platform_device`?**

> The Linux Device Tree parser, automatically during boot. The driver never
> allocates it \u2014 it just receives the `pdev` pointer in `probe()`.

**Q3. Who calls `probe()`?**

> The platform bus, after it matches a registered platform driver's
> `of_match_table` with a platform device's `compatible` string.

**Q4. Does `module_init()` initialize the hardware?**

> No. `module_init()` only calls `platform_driver_register()` to register the
> driver. Actual hardware initialization happens inside `probe()`, once per matched
> device.

**Q5. What is the purpose of the `compatible` string?**

> It is the **matching key** between a Device Tree node and a driver. When they
> match, the platform bus binds them and invokes `probe()`.

**Q6. Why doesn't `probe()` run immediately after `insmod`?**

> `probe()` runs only when Linux finds a matching DT node (matching `compatible`).
> If no matching device exists, the driver registers but `probe()` is never called.

**Q7. What happens if `compatible` doesn't match?**

> The module loads successfully, but `probe()` is never called and the hardware is
> never initialized \u2014 a common source of "my driver loads but does nothing" bugs.

**Q8. How do you pass your private structure from `probe()` to `remove()`?**

> Use `platform_set_drvdata(pdev, mydev)` in `probe()` and
> `platform_get_drvdata(pdev)` in `remove()` \u2014 the platform equivalent of
> `file->private_data`.

**Q9. Why is the Device Tree preferred over hardcoding hardware details?**

> It keeps the driver generic and board-independent. Board-specific data (GPIO
> numbers, addresses, IRQs) lives in the DT, so one driver works across many boards
> without code changes.

**Q10. Why use `devm_*` APIs?**

> They automatically free the allocated resources when the device is removed,
> avoiding manual cleanup in `remove()` and eliminating a common class of leak bugs.

---

## 15. Next Build (full example)

A complete example will include:

- Device Tree node (`.dts`)
- Platform driver (`gpio_button_platform.c`)
- `probe()` and `remove()`
- Reading the GPIO from the Device Tree
- Requesting the interrupt
- Testing it on the STM32MP257 board

Can probe() execute multiple times?

Answer:
Yes. Once for each matching platform device.

Q5

Can one driver control 10 devices?

Answer:
Yes. probe() is called once per device instance.

Q6

Difference between file->private_data and platform_set_drvdata()?

| file->private_data       | platform_set_drvdata      |
| ------------------------ | ------------------------- |
| Per file open            | Per device                |
| Lifetime until `close()` | Lifetime until `remove()` |
