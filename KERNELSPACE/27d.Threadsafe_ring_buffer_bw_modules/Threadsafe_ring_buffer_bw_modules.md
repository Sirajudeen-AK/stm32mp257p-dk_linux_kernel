
## Architecture
                Producer (Module A)
                       |
                  ring_push()
                       |
               spin_lock_irqsave()
                       |
                 Write Packet
                       |
                  spin_unlock()
                       |
              wake_up_interruptible()
                       |
                       V
            +-----------------------+
            |    Ring Buffer        |
            +-----------------------+
                       ^
                       |
              wait_event_interruptible()
                       |
                  ring_pop()
                       |
                 Consumer Thread


Why Spinlock?
Suppose Producer and Consumer run on different CPUs.

Without
	spin_lock_irqsave()

Both may update
	head
	tail
simultaneously.

Result
	Packet Loss
	Corruption
	Random Crash



Why Wait Queue?
Without
	wait_event_interruptible()

Consumer does
	while (1) {
		ring_pop();
	}

CPU usage	->	100%
Busy waiting.

With Wait Queue
	Sleep
	↓
	Wake
	↓
	Read
	↓
	Sleep
CPU usage
Almost zero.


Interview Questions
1. Why use a spinlock instead of a mutex?

A spinlock is suitable because the critical section is very short and can be used in atomic contexts (including interrupt handlers). A mutex may sleep and cannot be used there.

2. Why use a wait queue?

To block the consumer efficiently until data is available, avoiding busy waiting and unnecessary CPU usage.

3. Why wake the consumer after releasing the spinlock?

Because the shared data is already consistent. Waking a sleeping task while still holding the lock increases contention and can hurt performance.

4. Where is this design used?
Network RX/TX queues
CAN receive queues
USB endpoint queues
Audio capture/playback
Storage request queues
Character drivers