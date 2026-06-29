# Program 18 — Kthread + IRQ (Dedicated Thread + Wait Queue)

## 1. Goal

Run a **dedicated kernel thread** that sleeps until the ISR signals it, processes
the event, then sleeps again. This is the model used by drivers that need a
continuous, long-running processing loop (e.g. a CAN receive loop).

---

## 2. Why a Kthread Instead of a Workqueue?

```
CAN Interrupt
   ↓
Wake Thread
   ↓
Receive Frame
   ↓
Process
   ↓
Wait Again      (repeats forever)
```

A kthread is ideal because it **owns a dedicated execution loop**. A workqueue is
better for short, independent jobs; a kthread suits long-running background
processing tied to one device.

---

## 3. Flow

```
insmod
   ↓
probe()
   ↓
kthread_run()  → Kernel thread created
   ↓
Thread sleeps  (wait_event_interruptible)
   ↓
Button Press
   ↓
ISR: data_ready = true; wake_up_interruptible()
   ↓
Kernel thread wakes
   ↓
Process button (msleep, mutex, SPI...)  ← can sleep
   ↓
Sleep again  (loop back)
```

---

## 4. Code Walkthrough

```c
struct my_device {
    int gpio, irq;
    struct task_struct *thread;
    wait_queue_head_t waitq;
    bool data_ready;
};

static int thread_fn(void *arg)
{
    struct my_device *mydev = arg;
    while (!kthread_should_stop()) {
        wait_event_interruptible(mydev->waitq,
                mydev->data_ready || kthread_should_stop()); // sleep until work
        if (kthread_should_stop())
            break;
        mydev->data_ready = false;
        msleep(2000);                 // slow processing — allowed in a kthread
    }
    return 0;
}

static irqreturn_t button_irq(int irq, void *dev)
{
    struct my_device *mydev = dev;
    mydev->data_ready = true;         // 1. set flag BEFORE waking
    wake_up_interruptible(&mydev->waitq); // 2. wake the thread
    return IRQ_HANDLED;
}

mydev->thread = kthread_run(thread_fn, mydev, "button_thread"); // in probe()
kthread_stop(mydev->thread);                                    // in remove()
```

---

## 5. The Three IRQ Bottom-Half Models (16 vs 17 vs 18)

| Threaded IRQ (16)     | Workqueue (17)         | Kthread (18)                |
| --------------------- | ---------------------- | --------------------------- |
| `request_threaded_irq`| `schedule_work()`      | `kthread_run()` + wait queue |
| Dedicated IRQ thread  | Shared worker thread   | Dedicated continuous loop   |
| Per-interrupt         | Job-based              | Long-running background     |
| Stop via free_irq     | `cancel_work_sync()`   | `kthread_stop()`            |

---

## 6. Commands / Testing

```bash
sudo insmod kthread_IRQ.ko
dmesg -w               # press button → ISR then "Thread Processing Button Event"
sudo rmmod kthread_IRQ
```

---

## 7. Interview Questions & Answers

**Q1. Difference between a kthread and a workqueue?**

| Workqueue              | kthread                                     |
| ---------------------- | ------------------------------------------- |
| Shared worker threads  | Dedicated thread                            |
| Job-based              | Continuous loop                             |
| `schedule_work()`      | `kthread_run()`                             |
| `cancel_work_sync()`   | `kthread_stop()`                            |
| Good for deferred work | Good for long-running background processing |

> Use a workqueue for short, fire-and-forget jobs; use a kthread when you need an
> always-on loop dedicated to one device.

**Q2. Why use `wait_event_interruptible()`?**

> Without it the thread would busy-loop (`while(1){ if(data_ready)... }`) and waste
> CPU. The wait queue makes the thread sleep until work arrives, consuming
> essentially zero CPU while idle, and stays interruptible by signals.

**Q3. Why call `kthread_should_stop()`?**

> `kthread_stop()` sets a stop flag and wakes the thread. Checking
> `kthread_should_stop()` lets the loop exit cleanly so `rmmod` doesn't leave a
> running thread behind.

**Q4. Difference between `kthread_should_stop()` and a manual `atomic_t` flag?**

> Both signal the thread to exit, but `kthread_should_stop()` is built into the
> kthread API: `kthread_stop()` sets the flag, **wakes the thread, and blocks until
> it has actually returned** — guaranteeing a safe, synchronized shutdown. A manual
> `atomic_t` only flips a value; you must also wake the thread yourself and add
> your own wait to confirm it exited, which is more error-prone. Prefer
> `kthread_should_stop()`/`kthread_stop()` for clean teardown.

**Q5. Why set `data_ready = true` before `wake_up_interruptible()`?**

> The woken thread re-checks the condition. If the flag isn't set first, it wakes,
> finds nothing to do, and sleeps again — losing the event. Always set the
> condition, then wake.

**Q6. Why include `kthread_should_stop()` in the wait condition?**

> So the thread also wakes when stopping, even with no button event. Otherwise
> `kthread_stop()` could block forever waiting on a thread stuck in `wait_event`.