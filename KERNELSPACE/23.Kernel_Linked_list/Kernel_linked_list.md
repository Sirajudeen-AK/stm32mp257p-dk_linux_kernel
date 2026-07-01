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

# Interview Questions

1. Why intrusive lists instead of external nodes?
- Better cache locality.
- No extra allocation.
- One object can belong to multiple lists by embedding multiple `list_head`s.

2. Difference between `list_add()` and `list_add_tail()`?
- Head insertion (stack) vs tail insertion (queue).

3. Is `list_del()` enough?
- No. It only unlinks.

4. Why use `_safe()` iteration?
- Allows deletion during traversal.

5. What happens if `kfree()` is called before `list_del()`?
- Corrupt list and dangling pointers.

6. Time complexity?
- Insert/Delete: O(1)
- Search: O(n)

7. Which macro retrieves parent object?
- `container_of()`.

8. Why mutex?
- Protect concurrent access.

9. Can one object belong to multiple lists?
- Yes, embed multiple `struct list_head` members.

10. Difference between intrusive and non-intrusive linked lists?
- Intrusive embeds list metadata in the object.

11. Why doubly linked instead of singly linked?
- O(1) deletion with only node pointer.

12. Why is list head circular?
- Eliminates NULL checks and simplifies edge cases.

13. Common bugs?
- Double delete
- Forgetting INIT_LIST_HEAD()
- Free before unlink
- Modifying while iterating without `_safe()`

14. Debugging tips
- Enable CONFIG_DEBUG_LIST.
- Check for use-after-free with KASAN.

15. Which kernel subsystems heavily depend on list_head?
- Memory management, scheduler, drivers, networking, VFS.

---
This guide is interview-focused and suitable for Linux kernel driver roles.
