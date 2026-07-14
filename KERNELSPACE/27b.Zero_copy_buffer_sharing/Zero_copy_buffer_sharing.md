Goal

Instead of calling exported functions only, Module A owns a memory buffer and Module B directly accesses it.

          +----------------------+
          |      Module A        |
          |----------------------|
          | kmalloc(4096)        |
          | shared_buffer ------+|
          +---------------------|-+
                                |
                                |
                                v
                    +----------------------+
                    |      Module B        |
                    |----------------------|
                    | Read                |
                    | Write               |
                    +----------------------+

Only one buffer exists in the system.
No copies.


Why?
Many kernel subsystems do exactly this.
Examples:
		Ethernet Driver
				│
				▼
			sk_buff
				│
				▼
			Bridge
				│
				▼
			VLAN
				│
				▼
			IP Stack

The same packet pointer travels through the stack.
Nobody copies the packet.



Interview Questions
1. Why share a pointer instead of copying?

To avoid unnecessary memory copies, reduce CPU usage, and improve throughput (zero-copy design).

2. How many copies exist?

Only one.

Both modules access the same memory.

3. Is locking required?

Yes, if multiple modules can modify the buffer concurrently. We'll introduce spinlocks/mutexes in the next programs.

4. Where is this used?
Networking (sk_buff)
CAN frameworks
Media pipelines
DMA buffers
Shared driver frameworks

5. Why not export the variable directly?

Instead of

EXPORT_SYMBOL(shared_buffer);

we export

get_shared_buffer();

because:
We keep ownership in Module A.
Module A can later add locking or validation.
It hides implementation details.
Easier to change internally without affecting users.

This follows encapsulation principles and is the preferred design.

6. Is this zero-copy?
Yes.
Both modules access the same allocated memory.

7. Is locking required?
yet.
Only Module B modifies the buffer.
If both modules read/write concurrently, synchronization is required (spinlock, mutex, RCU, etc.).

8. Where is this used?
	sk_buff in networking
	DMA buffers
	Video pipelines
	CAN frameworks
	Shared descriptor rings
	Audio drivers