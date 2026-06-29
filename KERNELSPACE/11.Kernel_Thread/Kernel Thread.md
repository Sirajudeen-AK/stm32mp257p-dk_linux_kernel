# Program 11 — Kernel Thread (kthread)

## 1. Why Do We Need a Kernel Thread?

Sometimes a driver must do work **periodically or in the background**, for example:

- Check hardware status every second
- Monitor CAN bus statistics
- Watch link status
- Collect debug information / heartbeat monitoring

You must **not** put an infinite loop inside `module_init()`:

```c
static int __init my_init(void)
{
    while (1) { ... }   // ❌ init never returns → insmod hangs forever
}
```

Instead, create a **kernel thread** that runs independently in the background.

---

## 2. Key APIs

```c
struct task_struct *t;

t = kthread_run(thread_fn, NULL, "my_thread");   // create + start in one call

// inside the thread function:
static int thread_fn(void *data)
{
    while (!kthread_should_stop()) {
        // ... do work ...
        msleep(1000);          // sleep 1 second (allowed in a kthread)
    }
    return 0;
}

// on rmmod:
kthread_stop(t);   // signals kthread_should_stop() to return true, waits for exit
```

---

## 3. Architecture / Lifecycle

```
insmod
  │
  ▼
module_init()
  │
  ▼
kthread_run()  ───▶  Kernel Thread created
                          │
                          ▼
                     thread_fn()
                          │
                          ▼
                while (!kthread_should_stop()) { do work; sleep; }
                          │
rmmod  ──────▶ kthread_stop()  ───▶ loop exits → thread terminates
```

---

## 4. CPU Affinity (binding a thread to a specific CPU)

By default a kthread can run on **any** CPU. To pin it to one core (useful for
real-time or per-core work), bind it **before** waking it:

```c
kthread_bind(my_thread, target_cpu);   // run only on target_cpu
```

- Without binding → the scheduler may move the thread across all CPUs.
- With binding → it executes only on the chosen CPU.

---

## 5. Real Driver Usage (split work: ISR + kthread)

```
ISR (interrupt context — must be fast, cannot sleep)
  │
  +--> receive frame
  +--> wake the kernel thread
            │
            ▼
Kernel Thread (process context — can sleep)
  │
  +--> heavy processing
  +--> logging
  +--> statistics
```

This keeps slow/heavy work **out of interrupt context**, where sleeping is illegal.

---

## 6. Interview Questions & Answers

**Q1. What is the difference between a process and a kernel thread?**

> A user process has its own user-space address space and runs user code. A kernel
> thread runs **entirely in kernel space**, has **no user address space**, and is
> scheduled like any task but only executes kernel code (e.g. `kworker`,
> `ksoftirqd`).

**Q2. How do you list kernel threads?**

> Use `ps -eLf` or `top`. Kernel threads typically show in square brackets, e.g.
> `[kworker/0:0]`, `[ksoftirqd/0]`, `[migration/0]`, `[rcu_preempt]`.

**Q3. Why is `msleep()` (or another sleep) needed inside the thread loop?**

> Without sleeping, `while (!kthread_should_stop())` spins continuously and burns
> 100% of a CPU. `msleep()` yields the CPU between iterations.

**Q4. Can a kernel thread sleep?**

> Yes. Because it runs in **process context**, it may call `msleep()`,
> `wait_event()`, `schedule()`, `mutex_lock()`, etc.

**Q5. Can an ISR sleep?**

> No. An ISR runs in **interrupt (atomic) context** where sleeping is forbidden.
> That is why heavy/sleeping work is handed off to a kernel thread or workqueue.

**Q6. How do you stop a kernel thread cleanly?**

> Call `kthread_stop(t)` from the exit path. It makes `kthread_should_stop()`
> return true so the loop exits, and it blocks until the thread has actually
> returned — ensuring no use-after-free during `rmmod`.

**Q7. How do you force a kthread to run on a specific CPU?**

> Call `kthread_bind(thread, cpu)` before the thread starts running. Without
> binding, the scheduler is free to run it on any CPU.