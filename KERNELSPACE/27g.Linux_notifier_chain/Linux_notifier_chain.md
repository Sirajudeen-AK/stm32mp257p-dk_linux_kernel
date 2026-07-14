Why Notifier Chain?

Instead of writing this ourselves

	Callback List
	↓
	register()
	↓
	unregister()
	↓
	notify()

Linux already provides
	Notifier Chain

which handles
	Callback list
	Registration
	Unregistration
	Traversal
	Priorities
	Synchronization


### Types of Notifier Chains
| Type                    | Sleep Allowed     | Lock Used | Usage                  |
| ----------------------- | ----------------- | --------- | ---------------------- |
| ⭐⭐⭐⭐⭐ Blocking Notifier | Yes               | Mutex     | Most common            |
| ⭐⭐⭐⭐ Atomic Notifier    | No                | Spinlock  | IRQ/Atomic context     |
| ⭐⭐⭐⭐ Raw Notifier       | Depends on caller | None      | Caller handles locking |
| ⭐⭐⭐ Bus SRCU Notifier   | Yes               | SRCU      | High concurrency       |


## Architecture
					Module A

			BLOCKING_NOTIFIER_HEAD()
						│
				register_client()
						│
				register_client()
						│
					notify()
						│
				+---------+---------+
				▼                   ▼
			Module B             Module C


## Return Values
Callback returns
	NOTIFY_OK		->		means		->		Continue
	NOTIFY_DONE		->		means		->		Nothing special happened
	NOTIFY_STOP		->		means		->		Stop executing remaining callbacks.

## Very common interview question.
Example

	Module C
	↓
	NOTIFY_STOP
	↓
	Module B  -> Never Executes

## Fail Tests
### Forget unregister
Unload Module B.
Notifier still holds its callback.
Future notification invokes an invalid function pointer, causing a crash.

### Register the same notifier twice
The same callback may execute twice (or registration may fail depending on the implementation and API usage). Avoid duplicate registrations.

### Calling a blocking notifier from interrupt context
blocking_notifier_call_chain() may sleep because it uses a mutex internally.
Calling it from interrupt context is incorrect.
Use an atomic notifier chain instead.

## Interview Questions
1. What is a notifier chain?

A kernel mechanism that lets multiple clients register callbacks and receive asynchronous event notifications.

2. Difference between our custom callback list and a notifier chain?

| Custom Callback         | Notifier Chain                    |
| ----------------------- | --------------------------------- |
| We implement everything | Kernel provides the framework     |
| No built-in priorities  | Supports priorities               |
| Manual synchronization  | Framework handles synchronization |
| Limited features        | Mature and widely used            |

3. Why use struct notifier_block?

It encapsulates the callback function, priority, and list linkage required by the notifier framework.

4. Difference between Blocking and Atomic Notifier?

| Blocking        | Atomic                   |
| --------------- | ------------------------ |
| Can sleep       | Cannot sleep             |
| Uses mutex      | Uses spinlock            |
| Process context | Interrupt/atomic context |

5. Where are notifier chains used?

	CPU hotplug
	Reboot notifications
	Power management
	Network device events
	USB subsystem
	Memory hotplug
	Clock framework