
Instead of one callback,
	Producer
	↓
	One Callback

	We'll support

	Producer
	↓
	Callback List
	↓
	Module B
	↓
	Module C
	↓
	Module D

This is the idea behind:
	Linux Notifier Chains
	Network Event Listeners
	USB Bus Notifications
	CPU Hotplug Notifications
	DRM Notifications



## Architecture

        	Module A

          register(cb)

          register(cb)

          register(cb)
                │
                ▼
        +------------------+
        | Callback List    |
        +------------------+
        | cb1              |
        | cb2              |
        | cb3              |
        +------------------+
                │
                ▼
           notify("Hello")
          /      |      \
        cb1     cb2     cb3


## Fail Tests
Forget mutex

Two modules register simultaneously.

Possible results:
	Corrupted linked list
	Memory corruption
	Kernel crash`

### Free node without removing from list
The list contains a dangling pointer.
Next notification traverses freed memory and may crash.


Interview Questions
1. Why use a linked list instead of a single callback pointer?

A linked list allows multiple independent modules to subscribe without imposing a fixed limit or wasting memory.

2. Why protect the callback list with a mutex?

Registration and unregistration modify the linked list and may sleep (kmalloc(GFP_KERNEL)), so a mutex is appropriate.

3. Why not use a spinlock?

Because registration/unregistration occur in process context and can sleep. Spinlocks are reserved for atomic contexts or very short non-sleeping critical sections.

4. Where is this pattern used?
USB device notifications
CPU hotplug events
Network interface state changes
DRM display notifications
Media subsystem events