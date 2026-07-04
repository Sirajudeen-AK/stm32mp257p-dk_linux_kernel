/*

memory Flow
Userspace

socket()

        │

sendmsg()

        │

-------------------------

Kernel

netlink_recv_msg()

        │

skb

        │

nlmsghdr

        │

payload

        │

Hello Kernel

        │

nlmsg_new()

        │

new skb

        │

Hello Userspace

        │

nlmsg_unicast()

-------------------------

Userspace

recvmsg()

        │

Hello Userspace


*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/netlink.h>
#include <net/sock.h>

#define NETLINK_USER 31

static struct sock *nl_sock;

static void send_reply(u32 pid, char *msg)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    int msg_size;

    msg_size = strlen(msg) + 1;

    skb = nlmsg_new(msg_size, GFP_KERNEL);
    if (!skb)
    {
        printk("Failed to allocate skb\n");
        return;
    }

	/*
	API - used to add a new netlink message to the socket buffer
	Arguments:
		skb - socket buffer
		portid - port id of the user space application (process id of the user space application)
		seq - sequence number of the message (0 for no sequence number, can be used for message ordering)
		type - type of the message (NLMSG_DONE for end of message, can be used for message type identification)
		payload - size of the message
		flags - flags for the message (0 for no flags, can be used for message control)
	*/
    nlh = nlmsg_put(skb,
                    0,
                    0,
                    NLMSG_DONE,
                    msg_size,
                    0);

    strcpy(nlmsg_data(nlh), msg);

	/*
	API - used to send the message to the user space application
	Arguments:
		nl_sk - netlink socket
		skb - socket buffer
		pid - process id of the user space application
	*/
    nlmsg_unicast(nl_sock,
                  skb,
                  pid);
}

static void netlink_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;

    char *msg;

    u32 pid;

    nlh = (struct nlmsghdr *)skb->data;

    msg = (char *)nlmsg_data(nlh);

    pid = nlh->nlmsg_pid;

    printk("Received from PID=%u\n", pid);
    printk("Message=%s\n", msg);

    send_reply(pid,
               "Hello Userspace");
}

static int __init my_init(void)
{
	/*
		struct netlink_kernel_cfg is used to configure the netlink socket in the kernel space. 
		It contains a single member, input, which is a pointer to a function that will be called 
		when a message is received on the netlink socket. In this case, the function netlink_recv_msg 
		will be called when a message is received.
	*/
    struct netlink_kernel_cfg cfg =
    {
        .input = netlink_recv_msg,
    };

	/*
	API - used to create a netlink socket in the kernel space
	Arguments:
		init_net - network namespace (init_net for the initial network namespace)
		NETLINK_USER - protocol type (NETLINK_USER for user-defined protocol)
		&cfg - configuration for the netlink socket (struct netlink_kernel_cfg)
	Returns:
		nl_sock - pointer to the created netlink socket (struct sock *)
	*/
    nl_sock = netlink_kernel_create(&init_net,
                                    NETLINK_USER,
                                    &cfg);

    if (!nl_sock)
        return -ENOMEM;

    printk("Netlink Loaded\n");

    return 0;
}

static void __exit my_exit(void)
{
	/*
	API - used to release the netlink socket in the kernel space
	Arguments:
		nl_sock - pointer to the netlink socket (struct sock *)
	*/
    netlink_kernel_release(nl_sock);

    printk("Netlink Removed\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Netlink Socket Example");