# Program 17 — Workqueue IRQ (ISR → Deferred Processing)

## 1. Goal

Handle a hardware interrupt with a very fast ISR, then push the slow work onto a
**workqueue** so it runs later in a shared kernel worker thread. This keeps the
CPU free to service more interrupts.

---

## 2. Why a Workqueue Instead of a Threaded IRQ?

The ISR finishes in a few microseconds and just calls `schedule_work()`:

```c
button_irq()
{
    schedule_work(...);   // ISR returns immediately
}
```

The CPU is free to handle more interrupts while a shared `kworker` thread does the
slow processing. Ideal when the heavy work is **background/deferred** rather than
strictly part of servicing the interrupt.

---

## 3. Flow

```
Button Press
      │
      ▼
GPIO Interrupt
      │
      ▼
ISR (button_irq) — very fast
   schedule_work(&mydev->work)
      │
      ▼
Return Immediately  → CPU free again
      │
      ▼
Kernel Worker Thread (kworker)   ← process context, CAN sleep
      │
      ▼
my_work_func()
   msleep(), mutex, SPI, I2C ...
```

---

## 4. Code Walkthrough

```c
struct my_device {
    int gpio, irq;
    struct work_struct work;          // 1. embed the work item
};

INIT_WORK(&mydev->work, my_work_func); // 2. bind handler (in probe)

static irqreturn_t button_irq(int irq, void *dev)
{
    struct my_device *mydev = dev;
    schedule_work(&mydev->work);       // 3. queue work, return fast
    return IRQ_HANDLED;
}

static void my_work_func(struct work_struct *work)
{
    struct my_device *mydev = container_of(work, struct my_device, work);
    msleep(3000);                      // 4. slow work — allowed here
}

// cleanup in remove():
cancel_work_sync(&mydev->work);        // 5. wait for any pending/running work
```

`container_of()` recovers the full device struct from the embedded `work` pointer
(same idea as `file->private_data` in earlier programs).

---

## 5. Threaded IRQ vs Workqueue

| Threaded IRQ                                          | Workqueue                              |
| ---------------------------------------------------- | -------------------------------------- |
| Dedicated IRQ thread                                 | Shared kernel worker thread            |
| Starts immediately after the IRQ                     | Runs when a worker thread is available |
| One thread per interrupt source                      | Worker threads are shared              |
| Best when work is directly tied to the interrupt     | Best for deferred/background work      |

---

## 6. Commands / Testing

```bash
sudo insmod Workqueue_IRQ.ko
dmesg -w               # press button → "ISR Executed" then "Workqueue started"
sudo rmmod Workqueue_IRQ
```

---

## 7. Interview Questions & Answers

**Q1. Can a workqueue sleep?**

> Yes. The work handler runs in **process context** on a kernel worker thread, so
> it may call `msleep()`, `mutex_lock()`, and do I²C/SPI transfers.

**Q2. Can a workqueue use a mutex?**

> Yes — process context allows blocking locks. (An ISR cannot, which is exactly why
> the work is deferred.)

**Q3. Can `schedule_work()` be called from an ISR?**

> Yes. That is its primary use case: the ISR queues work and returns immediately,
> moving the slow processing out of interrupt context.

**Q4. Why call `cancel_work_sync()` in `remove()`?**

> To guarantee no queued or running work continues after the module unloads. It
> waits for the handler to finish, preventing a use-after-free when the device
> struct is freed.

**Q5. Why embed `struct work_struct` in the device struct and use `container_of()`?**

> So each device has its own work item and the handler can recover the full device
> object from the `work` pointer — supporting multiple devices cleanly without
> globals.

**Q6. Difference between `schedule_work()` and a custom workqueue?**

> `schedule_work()` uses the shared `system_wq`. A custom queue
> (`create_workqueue` + `queue_work`) gives dedicated worker threads you own and
> can `destroy_workqueue()`, useful for isolation or heavy/ordered work.