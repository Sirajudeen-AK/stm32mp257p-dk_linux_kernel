
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/netlink.h>
#include <net/sock.h>

#define NETLINK_USER 31

#define MAX_CLIENTS    10

static u32 clients[MAX_CLIENTS];
static int total_clients;

static struct sock *nl_sock;

static void register_client(u32 pid)
{
    int i;

    for (i = 0; i < total_clients; i++)
    {
        if (clients[i] == pid)
            return;
    }

    if (total_clients >= MAX_CLIENTS)
    {
        printk("Client table full\n");
        return;
    }

    clients[total_clients++] = pid;

    printk("Client Registered PID=%u\n", pid);
}

static void send_broadcast(char *msg)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    int i;
    int size;

    size = strlen(msg) + 1;

    for (i = 0; i < total_clients; i++)
    {
        skb = nlmsg_new(size, GFP_KERNEL);

        if (!skb)
            continue;

        nlh = nlmsg_put(skb,
                        0,
                        0,
                        NLMSG_DONE,
                        size,
                        0);

        strcpy(nlmsg_data(nlh), msg);

        if (nlmsg_unicast(nl_sock,
                          skb,
                          clients[i]) < 0)
        {
            printk("PID=%u not reachable\n",
                   clients[i]);
        }
    }
}

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

     nlh = nlmsg_hdr(skb);

    msg = nlmsg_data(nlh);

    pid = nlh->nlmsg_pid;

	register_client(pid);

	printk("PID=%u Message=%s\n",
           pid,
           msg);

    if (!strcmp(msg, "broadcast"))
    {
        send_broadcast("Broadcast Message");
    }
    else
    {
        send_reply(pid,
                   "Hello Userspace");
    }
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