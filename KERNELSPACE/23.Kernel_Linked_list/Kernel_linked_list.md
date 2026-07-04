# Linux Kernel Linked List (`struct list_head`) – Complete Interview Guide

## Why does Linux use `struct list_head`?

The Linux kernel provides a generic intrusive doubly linked list implementation instead of every driver implementing its own linked list.

Advantages:
- Generic and reusable
- Doubly linked
- O(1) insertion
- O(1) deletion (when node is known)
- No extra allocation for list node
- Used across almost every kernel subsystem

---

## Structure Example

```c
struct student {
    int id;
    char name[32];
    struct list_head list;
};
```

### Memory Layout

```
+----------------------+
| id                   |
+----------------------+
| name[32]             |
+----------------------+
| struct list_head     |
|   next               |
|   prev               |
+----------------------+
```

The linked-list node is embedded inside the object (intrusive list).

---

## Flow

```
Head
 |
 v
A <-> B <-> C
```

Insert at tail:

```
Head
 |
 v
A <-> B <-> C <-> D
```

---

## Common APIs

| API | Purpose | Complexity |
|------|---------|------------|
| `list_add()` | Insert after head (LIFO) | O(1) |
| `list_add_tail()` | Insert at end (FIFO) | O(1) |
| `list_del()` | Remove node | O(1) |
| `list_empty()` | Check empty | O(1) |
| `list_for_each_entry()` | Traverse | O(n) |
| `list_for_each_entry_safe()` | Traverse while deleting | O(n) |

---

## Why `list_for_each_entry_safe()`?

Normal traversal:

```
A -> B -> C
     ^
   current
```

If `kfree(B)` happens first, the loop later accesses `B->next`, causing use-after-free.

Safe traversal stores the next pointer first:

```
next = C
delete B
move to C
```

---

## Correct deletion

```c
list_del(&node->list);
kfree(node);
```

Never reverse the order.

---

## Where used?

- Process list
- Scheduler
- PCI devices
- USB devices
- I2C devices
- Platform devices
- Network sockets
- Timers
- Workqueues
- Module list

---

# Interview Questions & Answers

**Q1. Why intrusive lists instead of external nodes?**

> Better cache locality, no extra allocation for the list node, and one object can
> belong to multiple lists by embedding multiple `list_head`s.

**Q2. Difference between `list_add()` and `list_add_tail()`?**

> Head insertion (stack/LIFO) vs tail insertion (queue/FIFO).

**Q3. Is `list_del()` enough?**

> No. It only unlinks the node; you must `kfree()` it if it was dynamically allocated.

**Q4. Why use `_safe()` iteration?**

> It saves the next pointer before the body runs, so you can delete the current
> node during traversal without a use-after-free.

**Q5. What happens if `kfree()` is called before `list_del()`?**

> The list is corrupted and neighboring nodes hold dangling pointers. Always
> `list_del()` first, then `kfree()`.

**Q6. Time complexity?**

> Insert and delete are O(1); search (as implemented) is O(n).

**Q7. Which macro retrieves the parent object?**

> `container_of()`, which `list_for_each_entry()` uses internally.

**Q8. Why mutex?**

> To protect the list from concurrent modification when multiple processes call
> read(), write(), or ioctl() at the same time.

**Q9. Can one object belong to multiple lists?**

> Yes, by embedding multiple `struct list_head` members in the object.

**Q10. Difference between intrusive and non-intrusive linked lists?**

> An intrusive list embeds the list metadata inside the object; a non-intrusive
> list allocates separate nodes that point to the data.

**Q11. Why doubly linked instead of singly linked?**

> O(1) deletion given only the node pointer, without walking from the head.

**Q12. Why is the list head circular?**

> It eliminates NULL checks and simplifies edge cases at the ends of the list.

**Q13. Common bugs?**

> Double delete, forgetting INIT_LIST_HEAD(), freeing before unlinking, and
> modifying the list while iterating without `_safe()`.

**Q14. Debugging tips?**

> Enable CONFIG_DEBUG_LIST and use KASAN to catch use-after-free.

**Q15. Which kernel subsystems heavily depend on list_head?**

> Memory management, scheduler, drivers, networking, and VFS.