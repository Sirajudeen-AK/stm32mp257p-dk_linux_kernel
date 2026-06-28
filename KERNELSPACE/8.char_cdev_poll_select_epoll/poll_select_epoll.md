# poll / select / epoll — Deeper Notes

These notes expand on two subtle, interview-favourite points behind
`poll`/`select`/`epoll`.

---

## 1. The Thundering Herd Problem

**Scenario:** 10 processes are all polling the same device. One message arrives.
**Question:** should all 10 wake up?

**Answer:** usually **no**.

```
One message arrives
      │
      ▼  wake_up_interruptible() wakes ALL 10
  app1 wakes ─┐
  app2 wakes  │  but only ONE can consume the single message
  ...         │  the other 9 wake, find nothing, sleep again  ← wasted work
  app10 wakes─┘
```

Waking many processes when only one can make progress wastes CPU and causes
lock contention. This is the **thundering herd problem**. Program 9 shows how
`wake_up_interruptible_nr()` helps limit how many waiters are woken.

---

## 2. Do Two Opens of the Same Device Get Separate Driver Instances?

**Scenario:** two applications both open `/dev/mychar`.

**Answer:** **No** — by default they **share** the driver's:

- global variables
- global buffers
- wait queues

The only thing that differs per open is their own `struct file *`.

```
app1 open("/dev/mychar") ┐
                          ├─▶ same driver, same globals, same wait queue
app2 open("/dev/mychar") ┘     (only struct file * differs)
```

This is exactly why later programs introduce:

```c
file->private_data
container_of()
```

to maintain **per-open / per-device** context instead of shared globals
(see Program 5).

---

## 3. Interview Questions & Answers

**Q1. What is the thundering herd problem?**

> When many processes wait on the same event and all are woken at once, but only
> one can actually proceed. The rest wake, find no work, and sleep again — wasting
> CPU cycles and causing contention.

**Q2. If two processes open the same `/dev` node, do they get isolated state?**

> No. They share the driver's global variables, buffers, and wait queues. Only
> their `struct file` differs. Per-open isolation requires `file->private_data`
> (and `container_of()` for per-device structures).

**Q3. How can you reduce the thundering herd effect?**

> Wake only as many waiters as can make progress — e.g.
> `wake_up_interruptible_nr(&wq, 1)` to wake a single waiter — or use exclusive
> waits (`wait_event_interruptible_exclusive`) so the kernel wakes one exclusive
> waiter at a time.