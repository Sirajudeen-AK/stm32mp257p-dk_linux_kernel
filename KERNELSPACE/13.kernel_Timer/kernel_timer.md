# Program 13 — Kernel Timer

## 1. Why Do We Need a Kernel Timer?

When you need to run something **after a certain amount of time** but you don't
want a thread sitting and sleeping. Examples:

- Turn an LED off after 5 seconds
- Retransmit a CAN frame if no ACK arrives within 100 ms
- Network timeout detection
- Watchdog-style periodic timeout
- Detect whether a device stopped responding

A timer schedules work based on **time** without dedicating a sleeping thread.

---

## 2. Key APIs

```c
struct timer_list my_timer;

timer_setup(&my_timer, timer_callback, 0);          // bind callback
mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000)); // fire after 5 s

static void timer_callback(struct timer_list *t)
{
    // runs in softirq context → must be short, MUST NOT sleep
}

del_timer_sync(&my_timer);   // cancel safely on cleanup
```

For a **periodic** timer, re-arm it from inside the callback:

```c
static void timer_callback(struct timer_list *t)
{
    // ... work ...
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000)); // run again in 1 s
}
```

---

## 3. Timer vs Delayed Work

| Kernel Timer              | Delayed Work                 |
| ------------------------- | ---------------------------- |
| Runs in **softirq** context | Runs in **process** context |
| **Cannot sleep**          | **Can sleep**                |
| Very lightweight          | Slightly heavier             |
| Good for timeout handling | Good for deferred processing |

> If your callback needs `msleep()`, `mutex_lock()`, or I2C/SPI transfers, do
> **not** use a timer — use delayed work instead (Program 12).

---

## 4. Running Sleeping Code After a Timer

Keep the timer callback short, and hand off sleeping work to a workqueue:

```
Timer expires (softirq, cannot sleep)
      │
      ▼
schedule_work()
      │
      ▼
Workqueue callback (process context, can sleep)
      ├── msleep()
      ├── mutex_lock()
      ├── i2c_transfer()
      └── spi_transfer()
```

This timer → workqueue pattern is common in production drivers.

---

## 5. Why `atomic_t` Instead of a Plain `bool`?

A plain `bool` shared between CPUs is not safe without synchronization:

|                    | `bool stopping`               | `atomic_t stop_flag`               |
| ------------------ | ----------------------------- | ---------------------------------- |
| CPU visibility     | may be cached in a register   | guaranteed visible to all CPUs     |
| Compiler reorder   | compiler may reorder reads    | `atomic_read` acts as a barrier    |
| Race-safe          | **No**                        | **Yes**                            |

---

## 6. Interview Questions & Answers

**Q1. Why use `mod_timer()` instead of `add_timer()`?**

> `mod_timer()` works whether the timer is currently inactive **or** already
> active — it sets/updates the expiry in one call. It's recommended because you
> don't have to first check and delete an active timer like you would with
> `add_timer()`.

**Q2. Can you sleep inside a timer callback?**

> No. Timer callbacks run in **softirq (atomic) context**, so calls like
> `msleep()` or `mutex_lock()` are illegal and will crash/oops the kernel.

**Q3. How do you run code that needs to sleep after a timer expires?**

> Keep the timer callback short and have it call `schedule_work()`. The workqueue
> handler runs in process context and may sleep (`msleep`, `mutex_lock`, I2C/SPI).

**Q4. Why use `atomic_t` instead of a plain `bool` for a stop flag?**

> A plain `bool` may be cached in a register and the compiler can reorder accesses,
> so other CPUs may not see updates reliably. `atomic_t` with `atomic_read`/
> `atomic_set` guarantees cross-CPU visibility and acts as a barrier, making it
> race-safe.

**Q5. Why use `del_timer_sync()` instead of `del_timer()` on cleanup?**

> `del_timer_sync()` waits until the callback has finished running on any other CPU
> before returning. This prevents the timer firing into freed memory during
> `rmmod` (a use-after-free).

**Q6. What is a jiffy?**

> `jiffies` is the kernel's tick counter incremented `HZ` times per second. Timer
> expiries are expressed relative to it, e.g.
> `jiffies + msecs_to_jiffies(1000)` for one second from now.