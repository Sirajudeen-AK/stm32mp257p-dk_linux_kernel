
This is more important than kthread for driver interviews.
Many production drivers use workqueues instead of creating their own kernel thread.


Problem
Suppose an interrupt occurs:

irq_handler()
{
    printk("IRQ");

    msleep(100);   // ❌ Illegal
}

ISR cannot sleep.
ISR should finish quickly.

Solution
Move heavy work out of ISR.
	IRQ
	|
	+--> schedule_work()
	|
	ISR returns quickly

	Later

	Kernel worker thread executes
	|
	+--> heavy processing
	+--> sleep allowed
	+--> I2C transaction
	+--> logging


Architecture
	ISR
	|
	v
	schedule_work()
	|
	v
	kernel workqueue thread
	|
	v
	work_handler()


How Linux Executes It
You didn't create any thread.
Linux already has worker threads:

kworker/0:0
kworker/1:0
kworker/2:0
...

Your work executes there.



Interview Question
Difference between kthread and workqueue?

| Kthread            | Workqueue            |
| ------------------ | -------------------- |
| Own thread         | Shared worker thread |
| More control       | Simpler              |
| More memory        | Less memory          |
| Long-running tasks | Short/medium tasks   |




schedule_delayed_work(wq, delay_start)   -> Delayed Workqueue (example: run after 5 seconds)
This is frequently used for:

	Link monitoring
	Periodic status checks
	Retry mechanisms
	PHY polling
	CAN bus health monitoring


	Why Delayed Work?
		Normal workqueue:
			schedule_work(&work);

			Runs ASAP.

			Sometimes you need:

				Retry after 1 second
				Check link after 5 seconds
				Poll hardware every 100ms
				Reinitialize device after timeout

			Instead of:

			msleep(5000);

			(which blocks a worker thread)

			use:
			schedule_delayed_work();



Q1. Difference between Timer and Delayed Work?

Timer:

timer callback

runs in softirq context.

Cannot sleep.

Delayed Work:

workqueue callback

runs in process context.

Can sleep.

Can:

mutex_lock()
msleep()
i2c_transfer()


Q2. Why not use msleep(5000)?

Bad:

work_handler()
{
    msleep(5000);
}

Blocks a worker thread for 5 seconds.

Good:

schedule_delayed_work()

Worker thread is free until execution time.





very Important:

1. Killing a Custom Workqueue (my_custom_wq)

Yes, you can absolutely kill/destroy a custom workqueue. When you create a workqueue using create_workqueue(), your driver owns it. To clean it up, you call destroy_workqueue(my_wq).
What happens behind the scenes:

    The kernel stops accepting any new work items into my_custom_wq.

    It forces the system to execute and finish any work items currently sitting in that queue.

    Once the queue is completely empty and current handlers exit, the kernel kills the dedicated worker threads and frees the memory.

2. Killing the System Workqueue (system_wq)

No, you cannot (and must never) kill the system workqueue.

The system workqueue (system_wq) is a global engine created by the Linux kernel core during boot setup. It is shared by hundreds of essential background drivers (storage, USB, input devices, network routing, and power management).
Why you cannot kill it:

    No Exported API: The kernel does not provide a destroy_workqueue(system_wq) function to developers.

    Instant Kernel Panic: If you somehow forced it to delete, the entire operating system would immediately freeze or crash (Kernel Panic), because vital system routines would suddenly lose their execution threads.

The Safe Execution Rules

If you ever need to stop work on either queue, remember these rules:
For Custom Queues:
C

// 1. Tell your infinite loop to stop using your atomic flag
atomic_set(&stop_work_flag, 1);

// 2. Wait for the current running execution to hit "break" and exit
cancel_work_sync(&my_work);

// 3. Destroy the queue infrastructure safely
destroy_workqueue(my_wq);

For System Queues:

If you submitted work to the default system queue using schedule_work(), you can only cancel your specific work item. You must leave the rest of the queue alone:
C

// 1. Tell your loop to stop using your atomic flag
atomic_set(&stop_work_flag, 1);

// 2. Cancel ONLY your work item from the system queue
cancel_work_sync(&my_work); 

// DO NOT call destroy_workqueue on the system queue!