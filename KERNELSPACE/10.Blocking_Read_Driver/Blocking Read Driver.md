# Program 10 — Blocking Read Driver

## 1. Goal

Implement a `read()` that **blocks (sleeps)** when there is no data, and
automatically returns once data becomes available. This is the classic behaviour
of devices like a GPS, sensor, or serial port.

---

## 2. Real-World Example (GPS Driver)

```c
fd = open("/dev/gps", O_RDONLY);
read(fd, buf, sizeof(buf));   // no data yet → process SLEEPS here
```

What happens behind the scenes:

```
read() called, no data yet
        │
        ▼  process sleeps (no CPU wasted)
A GPS packet arrives → ISR runs:
        gps_data_available = 1;
        wake_up_interruptible(&gps_wq);
        │
        ▼
sleeping read() wakes, copies data to user, returns
```

---

## 3. Flow

```
User space            Kernel space
----------            ------------
read()        ────▶   my_read()
                          │
                          ▼
                       no data?
                          │
                          ▼  SLEEP
                  wait_event_interruptible(&wq, data_ready)
                          ▲
                          │  wake_up_interruptible(&wq)
ISR / write()  ─────────┘
                          │
                          ▼
                  read() continues → copy_to_user() → return
```

---

## 4. Blocking Read vs poll()

| Blocking read()                | poll() / epoll()                       |
| ------------------------------ | -------------------------------------- |
| One syscall (`read`)           | Two syscalls (`poll` then `read`)      |
| Handles **one** fd             | Handles **many** fds at once           |
| Simple                         | Event-driven, scalable                 |
| Sleeps inside `read()`         | Sleeps inside `poll()`, then reads     |

Use a blocking read for a single, simple device. Use `poll`/`epoll` when one
program must watch many file descriptors.

---

## 5. Interview Questions & Answers

**Q1. Why use `wait_event_interruptible()` in a blocking read?**

> So the process **sleeps** instead of spinning in a loop. It gives up the CPU
> until the wake condition becomes true, and can still be interrupted by a signal.

**Q2. What wakes the sleeping read?**

> A `wake_up_interruptible(&my_wq)` call from the writer path or an ISR, made after
> the data-ready flag has been set.

**Q3. Why not use a `while(1)` busy loop instead?**

> Busy-waiting consumes 100% CPU doing nothing useful, wastes power, and starves
> other tasks. Sleeping on a wait queue is efficient and lets the scheduler run
> other work.

**Q4. Difference between `poll()` and blocking `read()`?**

> Blocking `read()` waits on a single fd with one syscall — simple but limited.
> `poll`/`epoll` let one process wait on many fds and react to whichever becomes
> ready, which is how event-driven applications (servers, GUIs) work.

**Q5. What happens if a signal arrives while the read is blocked?**

> `wait_event_interruptible()` returns `-ERESTARTSYS`. The driver should return
> from `read()` (often as `-ERESTARTSYS`), allowing the syscall to be restarted or
> the signal handled, so the process stays responsive/killable.

**Q6. What flag lets a read return immediately instead of blocking?**

> Opening with `O_NONBLOCK` (or setting it via `fcntl`). The driver should check
> `file->f_flags & O_NONBLOCK` and return `-EAGAIN` instead of sleeping when no
> data is available.