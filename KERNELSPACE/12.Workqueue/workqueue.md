# Program 12 \u2014 Workqueues

> This topic is **more important than kthreads for driver interviews**. Many
> production drivers use workqueues instead of creating their own kernel thread.

---

## 1. The Problem

An ISR runs in atomic context and **must finish fast** and **must not sleep**:

```c
irq_handler()
{
    printk("IRQ");
    msleep(100);   // \u274c illegal \u2014 sleeping in interrupt context crashes
}
```

We need to move slow/sleeping work **out of the ISR**.

---

## 2. The Solution: Defer Work to a Worker Thread

```
ISR (fast, atomic)
  \u2502
  +--> schedule_work(&my_work)   // queue the heavy work, then return immediately
  \u2502
  ISR returns quickly
        \u2502
        \u25bc (later, in process context)
Kernel worker thread runs work_handler()
  \u2502
  +--> heavy processing
  +--> sleeping allowed (msleep, mutex_lock, i2c_transfer, logging)
```

You don't create the worker thread \u2014 Linux already runs them as `kworker/0:0`,
`kworker/1:0`, etc. Your work executes on one of them.

---

## 3. Basic Workqueue API

```c
struct work_struct my_work;

INIT_WORK(&my_work, work_handler);   // bind handler

static void work_handler(struct work_struct *w)
{
    // runs in process context \u2192 may sleep
}

schedule_work(&my_work);             // queue onto the system workqueue
```

---

## 4. Delayed Work (run after a delay)

```c
struct delayed_work my_dwork;
INIT_DELAYED_WORK(&my_dwork, work_handler);

schedule_delayed_work(&my_dwork, msecs_to_jiffies(5000)); // run after 5 seconds
```

Common uses: link monitoring, periodic status checks, retry mechanisms, PHY
polling, CAN bus health monitoring.

**Why not just `msleep(5000)` inside the handler?**

```c
work_handler() { msleep(5000); }   // \u274c blocks a worker thread for 5 seconds
```

`schedule_delayed_work()` leaves the worker thread free until the time arrives \u2014
far more efficient than blocking it with a sleep.

---

## 5. Custom Workqueue vs System Workqueue

### Custom workqueue \u2014 you own it, you can destroy it

```c
struct workqueue_struct *my_wq = create_workqueue("my_custom_wq");
queue_work(my_wq, &my_work);
// cleanup:
destroy_workqueue(my_wq);
```

`destroy_workqueue()` stops accepting new work, **flushes** (runs) any pending
work, then kills the dedicated worker threads and frees memory.

### System workqueue \u2014 shared, you must NOT destroy it

`system_wq` (used by `schedule_work()`) is a global engine created by the kernel
at boot and shared by hundreds of drivers (storage, USB, input, networking,
power). There is **no API** to destroy it, and forcing it would cause a **kernel
panic**. You may only cancel **your own** work item.

---

## 6. Safe Cleanup Rules

**For a custom queue:**

```c
atomic_set(&stop_work_flag, 1);   // 1. tell your loop to stop
cancel_work_sync(&my_work);       // 2. wait for the running handler to finish
destroy_workqueue(my_wq);         // 3. tear down the queue
```

**For the system queue (work submitted via `schedule_work`):**

```c
atomic_set(&stop_work_flag, 1);   // 1. tell your loop to stop
cancel_work_sync(&my_work);       // 2. cancel ONLY your work item
// \u274c never call destroy_workqueue on the system queue
```

---

## 7. Interview Questions & Answers

**Q1. Difference between a kthread and a workqueue?**

| Kthread              | Workqueue              |
| -------------------- | ---------------------- |
| Your own thread      | Shared worker thread   |
| More control         | Simpler to use         |
| More memory          | Less memory            |
| Long-running tasks   | Short/medium tasks     |

> A kthread is a dedicated thread you create and manage. A workqueue runs your
> handler on shared kernel worker threads \u2014 simpler and lighter, ideal for short
> deferred work triggered from an ISR.

**Q2. Difference between a kernel timer and delayed work?**

> A **timer** callback runs in **softirq (atomic) context** and **cannot sleep**.
> **Delayed work** runs in **process context** and **can sleep** \u2014 it may call
> `mutex_lock()`, `msleep()`, `i2c_transfer()`, etc. Use a timer for lightweight
> timeouts, delayed work when the deferred action needs to sleep.

**Q3. Why not use `msleep(5000)` inside a work handler?**

> It blocks a shared worker thread for the entire duration, starving other work.
> `schedule_delayed_work()` schedules the handler to run later while keeping the
> worker free in the meantime.

**Q4. (Cisco/Harman/Bosch style) An interrupt occurs, but recovery should happen after 1 second. How do you implement it?**

> From the ISR call `schedule_delayed_work()` with a 1-second delay. The ISR
> returns immediately, and the recovery procedure runs later in the delayed-work
> handler in process context (where it may sleep).

**Q5. Can you destroy the system workqueue?**

> No. `system_wq` is a global, shared kernel resource with no destroy API.
> Destroying it would crash the system. You may only cancel your own work item
> with `cancel_work_sync()`.

**Q6. What does `cancel_work_sync()` do?**

> It cancels a pending work item and, if it is already running, **blocks until the
> handler finishes**. This guarantees no handler is still executing before you
> free resources \u2014 preventing use-after-free during cleanup.