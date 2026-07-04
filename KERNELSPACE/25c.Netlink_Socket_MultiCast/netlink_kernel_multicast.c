#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <net/genetlink.h>

/*
	used to define a set of commands for a generic netlink family.
*/
enum {
    MY_CMD_UNSPEC,	// Unspecified command, used as a placeholder.
    MY_CMD_NOTIFY,	// Command for notifications, used to send notifications from kernel to user space.
    __MY_CMD_MAX,	// Maximum command identifier, used to define the upper limit of valid commands.
};
#define MY_CMD_MAX (__MY_CMD_MAX - 1)	// Maximum valid command identifier

/*
	used to define a set of attributes for a generic netlink family.
*/
enum {
    MY_ATTR_UNSPEC,	// Unspecified attribute, used as a placeholder.
    MY_ATTR_MSG,	// Attribute for a message string, used to send a message from kernel to user space.
    __MY_ATTR_MAX,	// Maximum attribute identifier, used to define the upper limit of valid attributes.
};
#define MY_ATTR_MAX (__MY_ATTR_MAX - 1)	// Maximum valid attribute identifier

/*
	used to define a multicast group identifier for the generic netlink family.
*/
enum {
    MY_MCGRP_EVENTS,	// Multicast group identifier for events, used to categorize messages sent to user space.
};

/*
	used to define a policy for validating netlink attributes. 
	A policy is a set of rules that specify how attributes should be interpreted and 
	validated when received in a netlink message. 

		enum nla_type type; // Type of the attribute (e.g., NLA_U8, NLA_U16, NLA_STRING, etc.)
		int len;            // Length of the attribute (for fixed-length types)
*/
static struct nla_policy my_policy[MY_ATTR_MAX + 1] = {
    [MY_ATTR_MSG] = { .type = NLA_NUL_STRING },
};

/*
	used to define a multicast group in the kernel. A multicast group is a mechanism that allows 
	multiple user-space applications to receive messages from the kernel simultaneously. 

		const char *name; // Name of the multicast group
*/
static const struct genl_multicast_group my_mcgrps[] = {
    [MY_MCGRP_EVENTS] = {
        .name = "events",
    },
};

/*
	used to define a generic netlink family in the kernel. 
	A generic netlink family is a collection of commands and attributes that can be used 
	for communication between user space and kernel space.

		const char *name; // Name of the generic netlink family
		u8 version;       // Version of the family
		u8 maxattr;       // Maximum attribute type supported by the family
		struct module *module; // Pointer to the module that owns this family
		const struct genl_multicast_group *mcgrps; // Array of multicast groups associated with this family
		int n_mcgrps;     // Number of multicast groups in the mcgrps array

*/
static struct genl_family my_family = {
    .name = "MY_FAMILY",
    .version = 1,
    .maxattr = MY_ATTR_MAX,
    .module = THIS_MODULE,
    .mcgrps = my_mcgrps,
    .n_mcgrps = ARRAY_SIZE(my_mcgrps),
};

static void send_multicast_message(void)
{
    struct sk_buff *skb;
    void *hdr;
    char *text = "Button Pressed";

	/*
	API - genlmsg_new
		Allocates a new generic netlink message buffer. This function is used to create a new socket buffer (sk_buff) for sending a generic netlink message. 
		Arguments:
		- size: The size of the message buffer to allocate.
		- flags: Allocation flags (e.g., GFP_KERNEL).
		Returns:
		- A pointer to the allocated sk_buff, or NULL on failure.
	*/
    skb = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
    if (!skb)
        return;

	/*
	API - genlmsg_put
		Prepares a generic netlink message header. This function is used to initialize the header of a generic netlink message in the provided socket buffer (sk_buff). 
		Arguments:
		- skb: A pointer to the sk_buff where the message will be constructed.
		- portid: The port ID of the sender (usually 0 for kernel).
		- seq: The sequence number of the message (usually 0 for kernel).
		- family: A pointer to the struct genl_family that describes the family of the message.
		- flags: Message flags (e.g., 0 for no flags).
		- cmd: The command identifier for the message.
		Returns:
		- A pointer to the start of the message header, or NULL on failure.
	*/
    hdr = genlmsg_put(skb,
                      0,
                      0,
                      &my_family,
                      0,
                      MY_CMD_NOTIFY);

    if (!hdr) {
        nlmsg_free(skb);
        return;
    }

	/*
	API - nla_put_string
		Adds a string attribute to a netlink message. This function is used to add a string attribute to the provided socket buffer (sk_buff) for a netlink message. 
		Arguments:
		- skb: A pointer to the sk_buff where the attribute will be added.
		- attrtype: The type identifier for the attribute.
		- value: The string value to be added as an attribute.
		Returns:
		- 0 on success, or a negative error code on failure.
	
	*/
    nla_put_string(skb,
                   MY_ATTR_MSG,
                   text);

	/*
	API - genlmsg_end
		Finalizes a generic netlink message. This function is used to complete the construction of a generic netlink message in the provided socket buffer (sk_buff). 
		Arguments:
		- skb: A pointer to the sk_buff where the message is being constructed.
		- hdr: A pointer to the start of the message header (returned by genlmsg_put).
		Returns:
		- None.
	
	*/
    genlmsg_end(skb, hdr);

	/*
	API - genlmsg_multicast
		Sends a multicast message to all listeners of a specific multicast group. This function is used to send a generic netlink message to all user-space applications that have subscribed to a specific multicast group. 
		Arguments:
		- family: A pointer to the struct genl_family that describes the family of the message.
		- skb: A pointer to the sk_buff containing the message to be sent.
		- portid: The port ID of the sender (usually 0 for kernel).
		- group: The multicast group identifier to which the message will be sent.
		- flags: Message flags (e.g., GFP_KERNEL).
		Returns:
		- 0 on success, or a negative error code on failure.
	*/
    genlmsg_multicast(&my_family,
                      skb,
                      0,
                      MY_MCGRP_EVENTS,
                      GFP_KERNEL);
}

static int __init my_init(void)
{
    int ret;

	/*
		API - genl_register_family
			Registers a generic netlink family with the kernel. This function is used to register a new generic netlink family, 
			which allows user-space applications to communicate with the kernel using the generic netlink protocol. 
		Arguments:
		- family: A pointer to a struct genl_family that describes the family being registered.
		Returns:
		- 0 on success, or a negative error code on failure.
	*/
    ret = genl_register_family(&my_family);
    if (ret)
        return ret;

    printk("Generic Netlink Loaded\n");

    /* Demo notification */
    send_multicast_message();

    return 0;
}

static void __exit my_exit(void)
{
	/*
		API - genl_unregister_family
			Unregisters a generic netlink family from the kernel. This function is used to remove a previously registered generic netlink family, 
			which stops user-space applications from communicating with the kernel using that family. 
		Arguments:
		- family: A pointer to a struct genl_family that describes the family being unregistered.
		Returns:
		- None.
	*/
    genl_unregister_family(&my_family);
    printk("Generic Netlink Removed\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Netlink Socket Example");