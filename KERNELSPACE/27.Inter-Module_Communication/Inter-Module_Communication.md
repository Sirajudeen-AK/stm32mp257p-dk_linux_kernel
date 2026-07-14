## Shared Memory

		+-----------------------+
		|      Module A         |
		|                       |
		| kmalloc(4KB)          |
		|       |               |
		+-------|---------------+
				|
				| pointer
				|
		+-------v---------------+
		|      Module B         |
		|                       |
		| Read/Write            |
		+-----------------------+


## Shared Ring Buffer
		Producer Module
		↓
		head++
		↓
		Ring Buffer
		↓
		tail++
		↓
		Consumer Module
Exactly how many networking drivers work.


## Callback Registration
Like the Linux kernel itself:

		register_listener()
		↓
		Module B callback stored
		↓
		Module A calls callback
No polling.


## Zero Copy Communication ⭐⭐⭐⭐⭐

Instead of
	Module A
	↓
	Copy
	↓
	Module B

We'll do
	Module A
	↓
	Pointer
	↓
	Module B

This is how high-performance drivers work.


## Interview Questions
	What is EXPORT_SYMBOL?
	Difference between EXPORT_SYMBOL and EXPORT_SYMBOL_GPL?
	How do two kernel modules communicate?
	How do they share memory?
	How do you avoid copying data?
	How do you prevent use-after-free when sharing pointers?
	How do you manage module dependencies?
	How does the networking stack pass packets between modules?

Interview Questions
1. Why use EXPORT_SYMBOL()?

To make a function or variable available to other loadable kernel modules.

2. What happens if the symbol isn't exported?

The dependent module fails to load with an Unknown symbol error.

3. Why can't Module A be unloaded while Module B is using it?

The kernel maintains a module reference count. Since Module B depends on Module A, Module A's reference count is non-zero, preventing it from being unloaded.

4. Difference between EXPORT_SYMBOL() and EXPORT_SYMBOL_GPL()?
EXPORT_SYMBOL() → available to all kernel modules.
EXPORT_SYMBOL_GPL() → available only to modules with a GPL-compatible license.
5. How does the kernel resolve exported symbols?

When a module is loaded, the module loader searches the kernel's exported symbol table (kallsyms) and patches the module's unresolved references with the actual function addresses.