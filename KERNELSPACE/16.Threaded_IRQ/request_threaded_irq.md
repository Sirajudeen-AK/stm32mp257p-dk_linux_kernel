# Program 16 — Threaded IRQ (request_threaded_irq)

## 1. Goal

Split interrupt handling into two parts: a tiny **top half** (hard IRQ) that runs
instantly, and a **bottom half** that runs in its own kernel thread where sleeping
is allowed. This is the cleanest way to do slow work (I²C/SPI, mutex, msleep)
triggered by a button press.

---

## 2. Why Threaded IRQ?

An ISR runs in **hard IRQ context** and must finish in microseconds — it cannot
sleep, take mutexes, or do I²C/SPI. But many devices need exactly that after an
interrupt. `request_threaded_irq()` solves this: the top half just says "wake the
thread", and the heavy work runs in the bottom-half **thread** which *can* sleep.

---

## 3. Flow

```
User presses button
      │
      ▼
Hardware IRQ
      │
      ▼
Top Half (top_irq)        ← hard IRQ, must be fast, no sleeping
   printk()
   return IRQ_WAKE_THREAD
      │
      ▼
Top half exits immediately  → CPU is free again
      │
      ▼
Kernel wakes the IRQ thread
      │
      ▼
Bottom Half (thread_irq)  ← thread context, CAN sleep
   msleep(), mutex_lock(), SPI, I2C, copy data
      │
      ▼
   return IRQ_HANDLED  (done)
```

---

## 4. Code Walkthrough

```c
// Top half: minimal work, then hand off
static irqreturn_t top_irq(int irq, void *dev)
{
    printk("Top Half\n");
    return IRQ_WAKE_THREAD;     // ask kernel to run the threaded handler
}

// Bottom half: runs in a dedicated thread → may sleep
static irqreturn_t thread_irq(int irq, void *dev)
{
    printk("Thread started\n");
    msleep(1000);              // legal here, illegal in top_irq
    printk("Thread finished\n");
    return IRQ_HANDLED;
}

// Registration (device-managed → auto-freed on remove)
devm_request_threaded_irq(&pdev->dev, irq,
                          top_irq,            // top half
                          thread_irq,         // bottom half (threaded)
                          IRQF_TRIGGER_FALLING,
                          "button_irq", mydev);
```

If you pass `NULL` for the top half, the kernel uses a default that always wakes
the thread — useful when you have no fast work to do at all.

---

## 5. Commands / Testing

```bash
sudo insmod request_threaded_irq.ko
dmesg -w                # press the button → "Top Half" then "Thread started/finished"
sudo rmmod request_threaded_irq
```

---

## 6. Interview Questions & Answers

**Q1. Why can't `msleep()` be used in an ISR?**

> A hard IRQ handler runs in atomic context with interrupts disabled and no
> backing process. The scheduler cannot switch tasks there, so sleeping would
> deadlock. Sleeping work must move to the threaded bottom half.

**Q2. What does `IRQ_WAKE_THREAD` do?**

> Returned from the top half, it tells the kernel to schedule the registered
> threaded handler. The thread then runs in process context where sleeping is
> allowed.

**Q3. What is the difference between the top half and bottom half?**

| Top Half            | Bottom Half           |
| ------------------- | --------------------- |
| Hard IRQ context    | Kernel thread context |
| Must be very fast   | Can be slow           |
| Cannot sleep        | Can sleep             |
| Cannot take mutexes | Can take mutexes      |

> The top half acknowledges the hardware instantly; the bottom half does the slow
> work afterwards so interrupts stay disabled for the shortest possible time.

**Q4. When would you use a threaded IRQ?**

> When interrupt processing needs operations that may sleep — I²C/SPI transfers,
> mutexes, `GFP_KERNEL` allocations, firmware access, or longer processing.

**Q5. What if you pass `NULL` as the top-half handler?**

> The kernel substitutes a default top half that simply returns `IRQ_WAKE_THREAD`,
> so the threaded handler always runs. Use this when there is no urgent work.

**Q6. Threaded IRQ vs scheduling a workqueue from a normal ISR?**

> A threaded IRQ gives a **dedicated** thread tightly bound to the interrupt and
> starts as soon as the IRQ fires. A workqueue shares worker threads and is better
> for deferred/background work not strictly tied to servicing the interrupt
> (compared in Program 17).