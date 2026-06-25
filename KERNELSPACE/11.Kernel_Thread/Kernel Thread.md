Why do we need a kernel thread?

Suppose your driver must do some work periodically:

Check hardware status every second
Monitor CAN bus statistics
Watch link status
Collect debug information
Heartbeat monitoring

You don't want:

while (1) {
    ...
}

inside init() because init never returns and module loading hangs.

Instead create a kernel thread.



Architecture

			insmod
			|
			v
			module_init()
			|
			v
			kthread_run()
			|
			v
			Kernel Thread Created
			|
			v
			thread_fn()
			|
			v
			while (!kthread_should_stop())
			{
				do work
				sleep
			}
			|
			v
			rmmod
			|
			v
			kthread_stop()
			|
			v
			Thread exits



Interview Questions
Q1. Difference between process and kernel thread?

	Process:
		User space
		Has user memory

	Kernel thread:
		Kernel space only
		No user address space


Q2. How to see kernel threads?
ps -eLf

or

top

Examples:

kworker
ksoftirqd
migration
rcu_preempt

All are kernel threads.


Q3. Why msleep()?

Without it:

while (!kthread_should_stop())

consumes 100% CPU.


Q4. Can kernel thread sleep?

Yes.

msleep()
wait_event()
schedule()
mutex_lock()

all can sleep.


Q5. Can ISR sleep?

No.
Kernel thread can sleep.
ISR cannot.


Real Driver Usage
Example CAN driver:

	ISR
	|
	+--> receive frame
	|
	+--> wake thread

	Kernel Thread
	|
	+--> heavy processing
	|
	+--> logging
	|
	+--> statistics

This avoids doing heavy work inside interrupt context.


Important:
without binding to target CPU it thread executes all the CPU's 
// Force the kthread to only execute on target_cpu-1
        kthread_bind(my_thread2, target_cpu);

but we bind to target then it executes only target CPU