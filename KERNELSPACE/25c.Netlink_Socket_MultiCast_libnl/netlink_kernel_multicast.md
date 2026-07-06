Architecture

        Kernel

      genlmsg_multicast()

              │
      Multicast Group

      ┌───────┼────────┐
      │       │        │
   Client1 Client2 Client3
   (joined)(joined)(joined)

   Only clients that join the multicast group receive the notification.

Generic Netlink requires:
	Resolve family ID
	Join multicast group
	Receive Generic Netlink message


Real Examples
WiFi
	Scan Complete
	↓
	Everyone interested gets notification.

USB
	USB Attached
	↓
	udev
	↓
	GUI
	↓
	Logger


## Interview Questions & Answers

**Q1. Why didn't we use NETLINK_USER multicast?**

> Because true multicast groups are provided by Generic Netlink, not by the simple custom Netlink protocol (NETLINK_USER) used in basic examples.

**Q2. Difference between Broadcast and Multicast?**

> Broadcast (our previous program) loops and sends individual unicasts. Multicast sends one message that the Netlink core distributes to all subscribed clients.

```
Broadcast                 Multicast
Loop                      Kernel
 ↓                         ↓
Unicast                   One multicast
 ↓                         ↓
Unicast                   Netlink Core
 ↓                         ↓
Unicast                   Subscribed Clients
(kernel sends            (networking subsystem
 individually)            distributes)
```

**Q3. Why is Generic Netlink preferred?**

> Dynamic family IDs, multicast groups, attribute-based messages (nla_put_*), an extensible protocol, and it is used by most modern Linux kernel subsystems.

**Q4. Which Linux subsystems use Generic Netlink?**

> nl80211 (Wi-Fi), cfg80211, ethtool, devlink, team, and many networking drivers.

**Q5. Cisco/Harman: If 500 applications are waiting for a hardware interrupt notification, would you use ioctl, Netlink unicast, broadcast, or multicast?**

> - ioctl: No, it's request/response and synchronous.
> - Unicast: No, you'd need 500 separate sends.
> - Broadcast (looped unicast): Works but is inefficient.
> - Generic Netlink Multicast: Best choice. One multicast operation notifies all subscribed clients, and only interested applications receive the event.



Install libnl
Ubuntu
	sudo apt install libnl-3-dev libnl-genl-3-dev

Compile
	gcc genl_multicast_client.c -o genl_client -lnl-3 -lnl-genl-3

Run

Terminal-1
	insmod my_genl.ko

Terminal-2
	./genl_client

Output
	Family ID = 30

	Group ID = 5

	Waiting for multicast messages...

Suppose kernel later does

	send_multicast_message();

Output
	Received : Button Pressed

Run another terminal

	./genl_client

Now both

Client-1

	Received : Button Pressed

and

Client-2

	Received : Button Pressed

receive the message.

No kernel changes.



What libnl does internally

This one line

	family_id = genl_ctrl_resolve(sock,
								FAMILY_NAME);

actually sends a Generic Netlink message to

	Kernel
	↓
	GENL_ID_CTRL
	↓
	What is the ID of

	MY_FAMILY ?

Kernel replies
	30


Similarly

	group_id =
	genl_ctrl_resolve_grp(...)

asks

	Kernel
	↓
	Which multicast group belongs

	to

	events ?

Kernel replies
	5


Then

	nl_socket_add_membership()

sends

	Kernel
	↓
	Join Group-5

Kernel records

	Client PID
	↓
	Group-5

Now whenever

	genlmsg_multicast()

is called,

the kernel automatically delivers the message to every socket that joined Group-5.


## Interview Questions & Answers — libnl Client

**Q1. Why use libnl?**

> It provides a high-level API for Generic Netlink, hiding low-level message construction, family resolution, attribute encoding, and parsing.

**Q2. What does genl_ctrl_resolve() do?**

> It queries the kernel's Generic Netlink controller to obtain the numeric family ID associated with a family name.

**Q3. Why not hardcode the family ID?**

> Family IDs are allocated dynamically when the family is registered. They can change across boots or systems, so applications should resolve them at runtime.

**Q4. What does genl_ctrl_resolve_grp() do?**

> It resolves the multicast group ID for a named Generic Netlink multicast group.

**Q5. What does nl_socket_add_membership() do?**

> It subscribes the Netlink socket to a multicast group so it receives multicast notifications sent to that group.

**Q6. Why is the client blocked in nl_recvmsgs_default()?**

> It waits for incoming Netlink messages. When the kernel multicasts an event, libnl receives the packet and invokes the registered callback (rx_callback()).

**Q7. What GFP flag should genlmsg_multicast() use and why does context matter?**

> Use GFP_KERNEL only in sleepable process context; use GFP_ATOMIC when multicasting from interrupt/softirq/atomic context. Passing GFP_KERNEL in atomic context can sleep and crash the kernel.

**Q8. How does the kernel know which sockets to deliver a multicast to?**

> When a client calls nl_socket_add_membership(), the Netlink core records that socket's membership in the group. genlmsg_multicast() then walks only the subscribed sockets — the driver keeps no per-client table.

**Q9. What is the difference between genlmsg_multicast() and genlmsg_multicast_allns()?**

> genlmsg_multicast() delivers within the current network namespace; the _allns() variant delivers across all network namespaces. Namespace scope matters for containerized systems.

**Q10. Why are attributes (nla_put_*) preferred over raw structs in Generic Netlink?**

> Attributes are TLV-encoded, so the protocol is extensible and version-tolerant: new fields can be added without breaking old clients, and parsing is validated by nla_policy. Raw structs break on layout/ABI changes.

**Q11. Why must the multicast group be registered with the family at init?**

> The group must exist and be assigned an ID before any client can resolve and join it. Groups are declared in the genl_multicast_group array and registered via the family registration.

**Q12. How does a client discover the group ID at runtime?**

> genl_ctrl_resolve_grp() queries the GENL_ID_CTRL controller for the numeric group ID by family+group name, since IDs are assigned dynamically at registration.


## Bottleneck / Performance Questions

**B1. Why is multicast far more efficient than looped unicast?**

> The kernel builds the message once and the Netlink core references/clones it to each subscribed socket, instead of allocating and sending N separate buffers. Sender cost is largely decoupled from the number of subscribers.

**B2. What happens if one subscriber's receive buffer overflows during multicast?**

> That socket's copy is dropped and it gets -ENOBUFS on its next recv (an overrun indication); other subscribers are unaffected. A slow consumer only hurts itself, unlike looped unicast where it can stall the loop.

**B3. Does multicast fully eliminate the per-subscriber cost?**

> No. The core still enqueues a reference to each subscribed socket, so delivery is still O(subscribers) inside the Netlink layer — but the expensive per-client allocation and driver-side bookkeeping are removed.

**B4. What is the scaling bottleneck for very high subscriber counts?**

> Socket receive-buffer memory (SO_RCVBUF × subscribers) and the per-socket enqueue walk. Bursty high-rate multicasts can pressure memory and cause ENOBUFS drops on slower consumers.

**B5. How do you tune the system for many multicast subscribers?**

> Increase SO_RCVBUF on clients, keep messages small and attribute-encoded, coalesce/rate-limit events, and ensure multicasts from atomic context use GFP_ATOMIC to avoid sleeping in the fast path.