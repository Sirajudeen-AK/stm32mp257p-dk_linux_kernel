Idea

Currently
User1 ----\
           \
User2 ------> Kernel
           /
User3 ----/

Kernel

↓

Reply only sender

Now we'll add a new command.

If one client sends

	broadcast

Kernel sends

	Broadcast Message

to
	User1
	User2
	User3



Important Observation

Although we call it Broadcast

internally
	for()
	↓
	nlmsg_unicast()
	↓
	Client1

	Client2

	Client3

This is actually

	Multiple Unicast

not true multicast.


## Interview Questions & Answers

**Q1. Why allocate one skb per client?**

> Because once nlmsg_unicast() queues or consumes an skb, that buffer cannot be reused for another destination. Each client needs its own outgoing message buffer.

**Q2. Why not send the same skb to everyone?**

> sk_buff ownership is transferred to the networking stack after a successful send. Reusing it would corrupt memory or cause double-free bugs.

**Q3. Is this true broadcast?**

> No. It's a loop that performs multiple unicasts.

**Q4. What happens if one client dies?**

> nlmsg_unicast() returns an error (typically because no Netlink socket exists for that port ID). The driver should eventually remove stale clients from its registration table.

**Q5. Why is this implementation inefficient for 10,000 clients? (Very Common)**

> Because it allocates a separate sk_buff and performs a separate send operation for every client. The work scales linearly with the number of clients. That's why the kernel provides Netlink Multicast Groups, which we'll implement in Program 25.3. There, the kernel sends one multicast message to a group, and the Netlink subsystem distributes it to all subscribed clients efficiently. This is how subsystems like nl80211 (Wi-Fi) and routing notifications are implemented.

**Q6. What is the time complexity of this "broadcast"?**

> O(n): one skb allocation and one nlmsg_unicast() per client. Cost grows linearly with the number of registered clients.

**Q7. Is the delivery atomic across all clients?**

> No. The loop delivers to clients one at a time, so some clients receive the message before others. A client that dies mid-loop simply fails its send while earlier clients already got the message. There is no all-or-nothing guarantee.

**Q8. What happens to ordering if the send loop sleeps?**

> Because each send is a separate operation, later clients see higher latency. If new messages are generated while looping, clients can observe events at different times, complicating ordering guarantees.

**Q9. How do you make this loop robust against a full receive buffer on one client?**

> Check the return value of nlmsg_unicast(); on -ENOBUFS/-EAGAIN, skip or mark that client rather than aborting the whole broadcast, so one slow client doesn't block the rest.

**Q10. Why must you hold a lock while iterating the client list here?**

> A client could register or unregister during the loop. Iterating an unlocked list risks touching freed entries. Take the list lock (or use RCU) around the iteration.


## Bottleneck / Performance Questions

**B1. Why does this design collapse at 10,000+ clients?**

> It performs 10,000 skb allocations and 10,000 send operations per event. Allocation pressure, per-send overhead, and lock hold time all scale linearly, causing high CPU use and latency. True multicast does one send and lets the Netlink core fan out.

**B2. What is the memory bottleneck of looped unicast?**

> Every iteration calls nlmsg_new(), so a single logical event allocates N buffers. Under frequent events this hammers the SLAB allocator and can exhaust memory faster than a single multicast message.

**B3. Where is the CPU spent, and how does multicast reduce it?**

> Most cost is repeated allocation + header build + queue-to-socket per client, all under a lock. Multicast builds the message once and the kernel clones/references it to subscribed sockets, cutting per-client work dramatically.

**B4. Can a single slow consumer stall the whole broadcast?**

> If the loop blocks or retries on a full buffer, yes — one slow client delays delivery to everyone. Non-blocking sends and skipping failed clients mitigate this, but it remains a fundamental limitation of looped unicast.


