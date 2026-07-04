Output

Open terminal-1

./netlink_user

Kernel

Client Registered PID=1354

Received from PID=1354

Message=Hello Kernel


Open terminal-2

./netlink_user

Kernel

Client Registered PID=1360

Received from PID=1360

Message=Hello Kernel


Run terminal-1 again

Received from PID=1354

Message=Hello Kernel

Notice

Client Registered

is NOT printed again.

Duplicate registration is prevented.



Memory

Initially

clients[]

Empty

↓

Terminal-1

clients[]

1354

↓

Terminal-2

clients[]

1354

1360

↓

Terminal-3

clients[]

1354

1360

1428

Kernel now knows every connected client.




## Interview Questions & Answers

**Q1. Why maintain a client table?**

> To keep track of connected userspace applications so the kernel can later notify one or more of them without requiring them to send another message first.

**Q2. Why check for duplicate PIDs?**

> A client may send many messages during its lifetime. Registering the same PID repeatedly wastes memory and causes duplicate notifications later.

**Q3. What happens if a registered process exits?**

> In this simple implementation, its PID remains in the table. Later, when the kernel tries to send to that PID, nlmsg_unicast() will fail because the destination socket no longer exists. Production drivers remove stale clients or require clients to unregister explicitly.

**Q4. Is getpid() always unique? (Very Common)**

> Only while the process exists. After a process exits, Linux may eventually reuse that PID for a different process. That's one reason production Netlink implementations include proper client lifecycle management instead of storing PIDs forever.

**Q5. Why is a fixed-size array (clients[10]) a poor design?**

> It caps the number of clients and silently drops registrations once full (array overflow if unchecked). Production code uses a dynamic structure such as a linked list, hash table (hashed on port ID), or IDR/XArray keyed by PID.

**Q6. How would you detect that a client has disappeared without polling?**

> Register a Netlink notifier (netlink_register_notifier) for NETLINK_URELEASE events. The kernel notifies you when a socket closes, so you can remove that port ID from the table immediately instead of discovering it on a failed send.

**Q7. Why is searching the client table for duplicates an O(n) operation, and how do you fix it?**

> A linear array scan on every message is O(n) per registration. Replacing it with a hash table or IDR keyed on the port ID makes lookup/insert effectively O(1).

**Q8. What does nlmsg_unicast() returning -ECONNREFUSED or -EAGAIN tell you?**

> It indicates the destination socket is gone (client exited) or its receive buffer is full. This is the trigger to prune stale clients or apply back-pressure.

**Q9. Why should the client table be protected by a lock?**

> The receive callback can run for different clients and the send/notify path can iterate the table concurrently. Without a spinlock/mutex you get races: torn reads, lost entries, or iterating a list being modified.


## Bottleneck / Scaling Questions

**B1. What is the main scalability bottleneck of a per-client table maintained by the kernel?**

> The kernel must store, search, and clean up state for every client. Memory and lookup cost grow with the client count, and stale entries accumulate if lifecycle handling is missing. Multicast groups push this bookkeeping into the Netlink core instead.

**B2. What happens under a "registration storm" (many clients connecting at once)?**

> With a linear array and a lock, each registration scans the table under the lock, so contention and O(n) scans serialize the storm. A hashed/IDR structure with fine-grained locking scales far better.

**B3. Why is storing raw PIDs long-term a correctness bottleneck, not just a performance one?**

> PIDs are reused after a process exits, so a stale entry can accidentally match a brand-new, unrelated process and deliver it messages meant for the old client. Lifecycle tracking (URELEASE) or per-socket identity avoids this.