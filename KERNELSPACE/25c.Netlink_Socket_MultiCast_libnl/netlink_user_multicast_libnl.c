#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#define FAMILY_NAME "MY_FAMILY"
#define GROUP_NAME  "events"

static int rx_callback(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh;
    struct genlmsghdr *ghdr;
    struct nlattr *attrs[2];	//	Array to hold parsed attributes, indexed by attribute type.

    nlh = nlmsg_hdr(msg);

    ghdr = nlmsg_data(nlh);

	/*
	API - nla_parse
		Parses netlink attributes from a netlink message. This function is used to extract attributes from a netlink message and store them in an array for easy access. 
		Arguments:
		- tb: An array of pointers to struct nlattr where the parsed attributes will be stored.
		- maxtype: The maximum attribute type to parse (usually the highest attribute type defined).
		- head: A pointer to the start of the attribute data in the message.
		- len: The length of the attribute data in the message.
		- policy: A pointer to a struct nla_policy that defines how to validate and interpret the attributes.
		Returns:
		- 0 on success, or a negative error code on failure.
	*/
    nla_parse(attrs,
              1,
              genlmsg_attrdata(ghdr,0),
              genlmsg_attrlen(ghdr,0),
              NULL);

    if(attrs[1])
    {
        printf("Received : %s\n",
               nla_get_string(attrs[1]));
    }

    return NL_OK;
}

int main(void)
{
    struct nl_sock *sock;

    int family_id;
    int group_id;

	/*
		API - nl_socket_alloc
		Allocates a new netlink socket. This function is used to create a new socket for communication with the 
		kernel using the netlink protocol. 
		Returns:
		- A pointer to the allocated netlink socket, or NULL on failure.
	*/
    sock = nl_socket_alloc();

    if(!sock)
    {
        printf("socket alloc failed\n");
        return -1;
    }

	/*
	 API - genl_connect
		Connects a netlink socket to the generic netlink protocol. This function is used to establish a connection 
		between the netlink socket and the generic netlink subsystem in the kernel.
		Arguments:
		- sock: A pointer to the netlink socket to connect.
		Returns:
		- 0 on success, or a negative error code on failure.
	*/
    if(genl_connect(sock))
    {
        printf("connect failed\n");
        return -1;
    }

	/*
	API - genl_ctrl_resolve
		Resolves the family ID for a given generic netlink family name. This function is used to obtain the 
		family ID associated with a specific generic netlink family, which is required for sending and receiving messages. 
		Arguments:
		- sock: A pointer to the netlink socket.
		- family_name: The name of the generic netlink family to resolve.
		Returns:
		- The family ID on success, or a negative error code on failure.
	*/
    family_id = genl_ctrl_resolve(sock,
                                  FAMILY_NAME);

    if(family_id < 0)
    {
        printf("family not found\n");
        return -1;
    }

    printf("Family ID = %d\n",
           family_id);

	/*
	API - genl_ctrl_resolve_grp
		Resolves the multicast group ID for a given generic netlink family and group name. This function is used to obtain the 
		multicast group ID associated with a specific generic netlink family and group, which is required for subscribing to multicast messages. 
		Arguments:
		- sock: A pointer to the netlink socket.
		- family_name: The name of the generic netlink family.
		- group_name: The name of the multicast group to resolve.
		Returns:
		- The multicast group ID on success, or a negative error code on failure.
	*/
    group_id = genl_ctrl_resolve_grp(sock,
                                     FAMILY_NAME,
                                     GROUP_NAME);

    if(group_id < 0)
    {
        printf("group not found\n");
        return -1;
    }

    printf("Group ID = %d\n",
           group_id);

	/*
	API - nl_socket_add_membership
		Adds a membership to a multicast group for a netlink socket. This function is used to subscribe the netlink socket to a specific multicast group, allowing it to receive messages sent to that group. 
		Arguments:
		- sock: A pointer to the netlink socket.
		- group: The multicast group ID to join.
		Returns:
		- 0 on success, or a negative error code on failure.
	*/
    nl_socket_add_membership(sock,
                             group_id);

	/*
	API - nl_socket_modify_cb
		Modifies the callback function for a specific netlink message type. This function is used to set or change the callback function that will be invoked when a specific type of netlink message is received. 
		Arguments:
		- sock: A pointer to the netlink socket.
		- cb_type: The type of callback to modify (e.g., NL_CB_VALID for valid messages).
		- cb_flags: Flags that control the behavior of the callback (e.g., NL_CB_CUSTOM for custom handling).
		- cb_func: A pointer to the callback function to be set.
		- cb_arg: A pointer to user-defined data that will be passed to the callback function.
		Returns:
		- 0 on success, or a negative error code on failure.
	*/
    nl_socket_modify_cb(sock,
                        NL_CB_VALID,
                        NL_CB_CUSTOM,
                        rx_callback,
                        NULL);

    printf("Waiting for multicast messages...\n");

    while(1)
    {
        nl_recvmsgs_default(sock);	
		/* 	Receives and processes incoming netlink messages using the default message 
		handling mechanism. This function blocks until a message is received, then invokes the appropriate callback 
		functions for processing the message. */
    }

	/*
	API - nl_socket_free
		Frees a netlink socket and releases associated resources. This function is used to clean up and deallocate a netlink socket when it is no longer needed. 
		Arguments:
		- sock: A pointer to the netlink socket to be freed.
		Returns:
		- None.
	*/
    nl_socket_free(sock);

    return 0;
}