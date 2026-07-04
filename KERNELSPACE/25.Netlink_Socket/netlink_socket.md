You'll learn:

	netlink_kernel_create()
	nlmsg_new()
	nlmsg_put()
	nlmsg_unicast()
	Kernel → Userspace communication
	Userspace → Kernel communication
	Message parsing
	Kernel callback
	Broadcasting (later)
	Multicast (later)

Netlink is a message-based IPC between kernel and userspace.

			Userspace-1
				▲
				│
				▼
		+----------------------+
		|      Netlink         |    <--- Socket
		+----------------------+
				▲
				│
				▼
			Kernel Module

Userspace API's
	socket()
	bind()
	sendmsg()
	recvmsg()
	close()

Kernel API's
	netlink_kernel_create()
	netlink_rcv_msg()
	nlmsg_new()
	nlmsg_put()
	nlmsg_unicast()
	netlink_kernel_release()


Message Format
Every Netlink message looks like
	+----------------------+
	| nlmsghdr             |
	+----------------------+
	| Payload              |
	+----------------------+

Kernel first reads
struct nlmsghdr

then
NLMSG_DATA(nlh)

returns your payload.


Communication
		Userspace

		"Hello Kernel"

				|

		sendmsg()

				|

		Kernel

		Callback()

		Receives

		"Hello Kernel"

				|

		Creates reply

		"Hello Userspace"

				|

		nlmsg_unicast()

				|

		Userspace

		recvmsg()

		prints reply


Important APIs

netlink_kernel_create()
Creates the kernel Netlink socket.

Equivalent of

register_chrdev()
or
alloc_chrdev_region()
for character drivers.


netlink_recv_msg()

Equivalent of

write()
in a character driver.
Whenever userspace sends data,
this callback executes.


nlmsg_new()
Allocates a socket buffer.

Equivalent of

kmalloc()
for a Netlink message


nlmsg_put()
Creates the Netlink header.
	+--------------+
	| nlmsghdr     |
	+--------------+
	| Payload      |
	+--------------+

nlmsg_data() -> Returns Payload pointer.

Equivalent to

copy_from_user()
except no copying from user buffer is needed here because the message is already inside the received socket buffer.


nlmsg_unicast()
Sends the packet back to
one
userspace process.


Comparison with Character Driver
| Character Driver       | Netlink                    |
| ---------------------- | -------------------------- |
| `/dev/mychar`          | No device node             |
| open()                 | socket()                   |
| write()                | sendmsg()                  |
| read()                 | recvmsg()                  |
| ioctl()                | Message-based commands     |
| Blocking read          | Asynchronous messaging     |
| One client (typically) | Multiple clients supported |


## Interview Questions & Answers — Basics

**Q1. What is Netlink?**

> A socket-based IPC mechanism used for communication between the Linux kernel and userspace.

**Q2. Why use Netlink instead of ioctl()?**

> Bidirectional, message-based communication that supports asynchronous notifications, multiple clients, and multicast groups, and is widely used by networking subsystems.

**Q3. Does Netlink require a /dev node?**

> No. It uses the socket API (AF_NETLINK) instead of the character device interface.

**Q4. Where is Netlink used in Linux?**

> Networking configuration, iproute2, udev, nl80211 (Wi-Fi), Netfilter, SELinux, the audit subsystem, and Generic Netlink families.

**Q5. What is the protocol number (NETLINK_USER 31)?**

> It's the Netlink protocol identifier used to distinguish different Netlink families. For learning, we use an unused user-defined protocol number such as 31.

## Interview Questions & Answers — sk_buff & Replies

**Q1. Why do we use struct sk_buff?**

> Because Netlink is built on the Linux networking stack, and messages are carried in socket buffers (sk_buff), just like network packets.

**Q2. Why allocate another skb for the reply?**

> The received skb belongs to the networking subsystem and must not be reused for transmitting a reply. A new buffer is allocated for the outgoing message.

**Q3. Why do we use nlmsg_put()?**

> It initializes the Netlink message header and reserves space for the payload.

**Q4. Where does nlmsg_pid come from?**

> It is the sender's Netlink port ID (commonly associated with the userspace socket), which is used as the destination for replies.

**Q5. Why use nlmsg_unicast()?**

> Because we are replying to exactly one userspace client. Later we'll extend this to broadcast and multicast.

## Interview Questions & Answers — Netlink vs Char Driver

**Q1. Why use Netlink instead of a character driver?**

> Because Netlink provides bidirectional, asynchronous, message-based communication between kernel and userspace. It naturally supports multiple clients and multicast, making it the preferred interface for networking and configuration subsystems.

**Q2. What is struct sk_buff?**

> It is the kernel's packet buffer object used throughout the networking stack. Netlink messages are carried inside sk_buff structures.

**Q3. Why is nlmsg_pid important?**

> It identifies the sender's Netlink socket (port ID). The kernel uses this value with nlmsg_unicast() to send the reply to the correct userspace process.

**Q4. Why doesn't Netlink need copy_from_user()?**

> Unlike a character driver's write(), the networking layer has already copied the message into kernel memory before invoking the Netlink callback. The driver accesses the payload through nlmsg_data().

**Q5. Why use sendmsg() and recvmsg() instead of write() and read()?**

> Netlink is a datagram/message-oriented protocol, not a byte stream. sendmsg() and recvmsg() preserve message boundaries and allow addressing and ancillary data.

**Q6. Is Netlink reliable?**

> Within the local machine, Netlink is generally reliable, but messages can still be dropped under resource pressure if buffers overflow. Applications should be prepared to handle such cases depending on the protocol.

**Q7. Where is Netlink used in Linux?**

> iproute2 (ip command), nl80211 (Wi-Fi configuration), Netfilter, SELinux, the audit subsystem, udev, traffic control (tc), and routing configuration.

**Q8. Why do we use NETLINK_USER = 31?**

> For learning, it's a user-defined Netlink protocol number. In production, many subsystems use predefined or Generic Netlink families rather than arbitrary protocol numbers.


Multiple UserSpace
How do you uniquely identify a Netlink client?
Answer
	Using its Netlink Port ID (nlmsg_pid).

Can kernel remember all clients?
Yes.
Store them.
Example
	static u32 clients[10];
	static int total_clients;

Whenever
	netlink_recv_msg()
is called,
store
	clients[total_clients++] = pid;
Now kernel knows
	1256
	1308
	1422
	1500


Kernel Broadcast
Kernel should notify everyone.

Instead of

nlmsg_unicast()

do
	for every client
			|
	nlmsg_unicast()

	1256

	1308

	1422

Everyone receives

Kernel
	for(i=0;i<total_clients;i++)
	{
		send_reply(clients[i],
				"Temperature Changed");
	}


Multicast
Instead of sending

	1256

	1308

	1422

individually,

Kernel sends

	Group-5

Anyone joined

	Group-5

receives.

        Group 5

    ▲     ▲       ▲

User1   User2   User3

Kernel
	One send
	↓
	Everyone receives

This is much more efficient.



Why Networking Drivers Use It
Suppose Wi-Fi state changes.

Kernel

	Link Down

Who wants it?

	Network Manager

	wpa_supplicant

	GUI

	Logger

Instead of sending

	4 unicasts

Kernel

	1 multicast

Done.


Interview Difference
Unicast
	Kernel
	↓
	One Client

Broadcast
	Kernel
	↓
	Loop
	↓
	All Clients

Multicast
	Kernel
	↓
	Group
	↓
	Subscribed Clients


## Interview Questions & Answers — Unicast / Broadcast / Multicast

**Q1. How does Netlink identify clients?**

> By the Netlink port ID (nlmsg_pid).

**Q2. Can two applications receive the same message?**

> Yes, via broadcast or multicast.

**Q3. Difference between Broadcast and Multicast?**

> Broadcast reaches everyone; multicast reaches only clients subscribed to the group.

**Q4. Why not use nlmsg_unicast() in a loop forever?**

> Because for many clients it becomes inefficient. Multicast lets the kernel distribute a single message to all members of a subscribed group.

**Q5. Can one userspace application communicate with multiple kernel modules using Netlink? (Very Common)**

> Yes. Different kernel modules can register different Netlink protocols or, more commonly in modern kernels, different Generic Netlink families. The userspace application selects the appropriate destination based on the protocol/family it wants to communicate with.


## More Interview Questions — Protocol Details

**Q9. What is the difference between AF_NETLINK and a normal AF_INET socket?**

> AF_NETLINK is an intra-host IPC address family (kernel ↔ userspace and userspace ↔ userspace). It never leaves the machine, has no IP addressing, and uses port IDs (usually the PID) instead of IP:port.

**Q10. What does nlmsg_pid = 0 mean?**

> A port ID of 0 refers to the kernel itself. Userspace messages destined for the kernel use pid 0, and the kernel is the implicit peer. Non-zero port IDs identify userspace sockets.

**Q11. How does Netlink preserve message boundaries?**

> Netlink is datagram/message oriented. Each sendmsg()/recvmsg() carries whole messages framed by struct nlmsghdr (nlmsg_len), unlike a TCP-style byte stream where the receiver must re-frame data.

**Q12. What are NLM_F_REQUEST, NLM_F_ACK and NLMSG_ERROR used for?**

> NLM_F_REQUEST marks a request, NLM_F_ACK asks the kernel to confirm, and the kernel replies with an NLMSG_ERROR message. An NLMSG_ERROR with error code 0 is actually the positive ACK; a non-zero code reports failure.

**Q13. What is the role of nlmsg_seq (sequence numbers)?**

> Sequence numbers let userspace correlate replies with requests and detect dropped/duplicate messages. They are application-managed, not enforced by the kernel.

**Q14. Which context does the Netlink receive callback run in?**

> netlink_kernel_create() input callbacks run in process context (via a workqueue-like path / the sender's context), so they may sleep. This differs from softirq/interrupt context and is why you can use nlmsg_new(GFP_KERNEL) there.

**Q15. Why do we call netlink_kernel_release() in module exit?**

> To drop the kernel socket and free associated resources. Forgetting it leaks the socket and leaves a dangling protocol registration, preventing clean reload of the module.


## Bottleneck / Performance Questions

**B1. What happens when the receiver's socket buffer is full?**

> nlmsg_unicast() (or the netlink send path) returns -ENOBUFS. The message is dropped, not queued indefinitely. A slow userspace reader is the classic Netlink bottleneck.

**B2. Is Netlink guaranteed reliable delivery?**

> No. It is reliable only in the sense that overflow is reported (ENOBUFS) rather than silently corrupting data. Under memory pressure or a full receive buffer, messages are dropped, so protocols must handle loss (resync/dump).

**B3. How do you handle a slow or stuck userspace consumer?**

> Either increase SO_RCVBUF, design the protocol to tolerate drops and re-request state (dump), or switch to multicast groups so the kernel does not track per-client buffers. Blocking the kernel to wait for a slow client is not acceptable.

**B4. Why can large Netlink transfers hurt performance?**

> Each message allocates an sk_buff and traverses the networking stack. High-frequency or large payloads cause many allocations, copies, and softirq/scheduling overhead. For bulk data, batch messages (NLM_F_MULTI) or use a shared-memory/mmap interface.

**B5. What is the concurrency bottleneck in the receive callback?**

> The input callback is effectively serialized per socket, so a heavy or blocking callback throttles all incoming messages on that socket. Offload heavy work to a workqueue and return quickly.

**B6. Why does allocating a reply skb per message matter at scale?**

> nlmsg_new() allocates memory on every reply. At thousands of messages/sec this dominates cost and can pressure the SLAB allocator. Reuse patterns, batching, or multicast reduce this overhead.


