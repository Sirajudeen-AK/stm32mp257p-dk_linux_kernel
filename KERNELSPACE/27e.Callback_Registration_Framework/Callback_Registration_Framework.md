This is one of the most common designs in the Linux kernel.

Examples:
	USB Core → USB Drivers
	PHY Framework → Ethernet Driver
	CPUFreq → Governors
	DRM → Connectors
	Network Stack → Protocol Modules
	CAN → Protocol Modules

Instead of Module B continuously asking Module A,

"Any data?"
"Any event?"

Module B registers a callback once, and Module A notifies it whenever something happens.



## Architecture

                Module A
           (Event Producer)

         register_callback()
                ▲
                │
                │
           Module B
                │
         event occurs
                │
                ▼
       callback(message)
                │
                ▼
            Module B


## Real Linux Example

Network Stack
		Ethernet Driver
		↓
		Packet Received
		↓
		Net Core
		↓
		TCP Callback
		↓
		UDP Callback
		↓
		Raw Socket Callback

Exactly the same idea.


## Forget unregister

If Module B unloads but Module A still keeps the callback pointer:

	Module B unloaded
	↓
	callback pointer still points to freed module text
	↓
	Module A calls callback
	↓
	Jump into invalid memory
	↓
	Kernel Oops

This is a classic bug and a frequent interview discussion point.



## Interview Questions
1. Why use callbacks instead of polling?

Polling repeatedly checks for events, wasting CPU cycles and increasing latency.

Callbacks are event-driven: the producer invokes the consumer only when an event occurs.

2. Why unregister callbacks?

To prevent stale function pointers. If a module unloads without unregistering, another module may later call a function that no longer exists, leading to a crash.

3. Can this support multiple clients?

Not in the current implementation.

We have:

static event_callback_t callback;

Only one callback can be stored.

Supporting multiple clients requires a list or array of callbacks.

4. How does the Linux kernel implement multi-client notifications?

The kernel provides Notifier Chains, which maintain a list of registered callbacks and invoke each one when an event occurs.