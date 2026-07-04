This is one of the most common interview topics because almost every kernel subsystem uses kobject.

Brief Explanation

A kobject represents a kernel object that can be exposed through sysfs.

	kobject
		│
		▼
	/sys/kernel/my_sysfs

Each attribute becomes a file.
value
name

When userspace executes:
	cat value

Kernel calls:
	value_show()

When userspace executes:
	echo 10 > value

Kernel calls:
	value_store()

No character device.

	No open()
	No read()
	No write()

Only show() and store() callbacks.



Examples:
	/sys/class/net/eth0
	/sys/class/gpio
	/sys/class/leds
	/sys/class/tty
	/sys/devices/...

All are built on kobject.

What you'll learn
	kobject_create_and_add()
	sysfs_create_group()
	__ATTR()
	show()
	store()
	Read/write from userspace using cat and echo
	Attribute permissions
	Cleanup

Architecture
Userspace

	     cat echo
			│
			▼
	/sys/kernel/my_sysfs/
			│
	+--------------------+
	| value              |
	| name               |
	+--------------------+
			│
		show()
		store()
			│
	Kernel Variables

Unlike character drivers:
	open()
	read()
	write()
	close()

Userspace directly accesses files under /sys.


## Interview Questions & Answers

**Q1. What is a kobject?**

> A core kernel object that provides object management, reference counting, and integration with sysfs.

**Q2. What is sysfs?**

> A virtual filesystem (/sys) that exposes kernel objects and their attributes to userspace.

**Q3. Difference between sysfs and procfs?**

> sysfs exposes kernel objects, devices, drivers, and configurable attributes (one value per file). procfs primarily exposes process information and general kernel/system information.

**Q4. Why does sysfs use show() and store() instead of read() and write()?**

> Sysfs files represent individual attributes, so the kernel uses standardized callbacks to read or update a single value.

**Q5. Why use __ATTR()?**

> It defines a sysfs attribute, including its name, permissions, and associated show()/store() callbacks.

**Q6. What does kernel_kobj mean?**

> It places the kobject under /sys/kernel/. Different parent kobjects place objects elsewhere in the sysfs hierarchy.

**Q7. Why does sysfs_create_group() exist?**

> It creates multiple related attributes in one call, making setup and cleanup simpler than creating each file individually.

**Q8. Where is sysfs used in real drivers?**

> Device drivers, network interfaces, LEDs, GPIO, USB, PCI, I²C, platform devices, power management, and sensor drivers.

**Q9. What is a ktype and why does every kobject need one?**

> A ktype (struct kobj_type) defines the object's sysfs_ops (the show/store dispatch), its default attributes, and its release() callback. Every kobject is tied to a ktype so the core knows how to display attributes and how to free the object when its refcount drops to zero.

**Q10. Why must a kobject have a release() method?**

> kobjects use reference counting. The object must only be freed when the last reference is dropped, not when the driver "thinks" it is done. release() is where the actual kfree() belongs, and the kernel warns if a ktype has no release().

**Q11. How does reference counting work with kobject_get() and kobject_put()?**

> kobject_get() increments the refcount (kref); kobject_put() decrements it. When the count reaches zero, the core automatically calls the ktype's release(). This prevents use-after-free when multiple subsystems hold the same object.

**Q12. What is a uevent and how does it relate to udev?**

> When a kobject is added or removed, the kernel can emit a uevent (via kobject_uevent()) over a Netlink socket. The userspace udev daemon listens for these events and creates/removes device nodes and runs rules. This is the link between sysfs and /dev.

**Q13. How do you notify userspace that a sysfs value changed?**

> Call sysfs_notify() on the attribute. A userspace program blocked in poll()/select() on that sysfs file wakes up, which is the standard push mechanism for sysfs (used e.g. by GPIO edge interrupts).

**Q14. What do the permission bits in __ATTR() control?**

> They set the file mode of the sysfs attribute. Read permission requires a show() callback; write permission requires a store(). A read-only attribute (0444) with a store() defined, or a writable attribute (0644) without a store(), is a common mistake.

**Q15. Difference between kobject_create_and_add() and manual kobject_init() + kobject_add()?**

> kobject_create_and_add() is a convenience helper that allocates a bare kobject with a default dynamic ktype and adds it in one call. Manual init/add is used when you embed the kobject inside your own structure and provide a custom ktype.

**Q16. Why must show()/store() handle PAGE_SIZE limits?**

> The sysfs buffer passed to show() is exactly one page (PAGE_SIZE). Writing more than a page overflows the buffer. Each attribute is meant to hold a single small value, not bulk data.


## Bottleneck / Design Trade-off Questions

**B1. Why is sysfs a bad choice for transferring large amounts of data?**

> Each sysfs file is limited to one page (PAGE_SIZE) per read/write and is designed for a single ASCII value. For bulk or streaming data you should use a char device, debugfs, mmap, or Netlink instead.

**B2. What is the "one value per file" rule and why does it matter?**

> sysfs convention is exactly one logical value per attribute file. Packing multiple values into one file breaks tooling (udev, libudev, scripts) and makes parsing fragile. It also hurts maintainability and ABI stability.

**B3. Can show()/store() block, and what is the consequence?**

> They run in process context and may sleep, but a slow or blocking show()/store() (e.g. taking a contended mutex or doing hardware I/O) stalls the calling userspace process and can serialize many readers. Keep them fast and avoid heavy locking in the hot path.

**B4. What locking pitfalls exist between store() and the rest of the driver?**

> Multiple userspace writers can call store() concurrently, and store() can race with interrupt/timer/work contexts that read the same variable. Unprotected shared state leads to torn reads/writes; you need a lock or atomic access.

**B5. What is the cost of creating thousands of sysfs attributes?**

> Each attribute consumes kernel memory (dentry/inode/kernfs node) and slows directory traversal. For large, dynamic data sets sysfs does not scale well; a single file, seq_file, or a different interface is preferred.

**B6. Why can a missing release() cause a memory leak or crash under load?**

> If release() is absent or frees the wrong thing, objects are never reclaimed (leak) or are freed while references remain (use-after-free). Under high add/remove churn this becomes a real stability bottleneck.