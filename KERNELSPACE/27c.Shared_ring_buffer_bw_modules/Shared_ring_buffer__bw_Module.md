This is the design used in:

	Network Drivers
	CAN Drivers
	Audio Drivers
	USB Drivers
	DMA Drivers
	UART Drivers

Instead of one shared buffer, we now have a ring (circular) buffer.


## Architecture
              Module A (Producer)
                      |
              Write Packet 1

              Write Packet 2

              Write Packet 3
                      |
                      V
      +-----------------------------------+
      | Packet0 Packet1 Packet2 Packet3   |
      |                                   |
      | head                      tail    |
      +-----------------------------------+
                      |
                      V
              Module B (Consumer)

                  Read Packet

## Circular Buffer

Initially
	head=0
	tail=0

After A
	head=1
	tail=0

After B
	head=2
	tail=0

After C
	head=3
	tail=0

Read one
	head=3
	tail=1

Read second
	head=3
	tail=2

Read third
	head=3
	tail=3

Empty
	head == tail



Interview Questions
1. Why use a ring buffer?
Fixed-size memory.
No shifting of elements.
O(1) enqueue/dequeue.
Excellent cache locality.
Ideal for producer-consumer systems.

2. Why leave one slot unused?

Using head == tail to indicate empty creates ambiguity if the ring becomes completely full. By leaving one slot unused, the conditions become simple:

Empty: head == tail
Full: (head + 1) % size == tail

Some implementations instead keep a count or a separate full flag.

3. Is this thread-safe?

No.

If multiple producers or consumers access it concurrently, synchronization is required.

We'll add locking in the next program.


4. Where is this used?
NIC RX/TX descriptor rings
CAN message queues
UART FIFOs
Audio streaming
USB endpoint queues
DMA descriptor rings