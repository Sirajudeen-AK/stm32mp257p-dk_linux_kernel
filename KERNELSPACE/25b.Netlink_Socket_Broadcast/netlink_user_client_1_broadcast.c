#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024

int main()
{
    int sock_fd;

	/* Initialize source and destination addresses */
    struct sockaddr_nl src_addr;
    struct sockaddr_nl dest_addr;

	/* Initialize netlink message header */
    struct nlmsghdr *nlh;

	/* Initialize I/O vector */
    struct iovec iov;

	/* Initialize message header */
    struct msghdr msg;

	/*
	API - used to create a netlink socket in the user space
	Arguments:
		AF_NETLINK - address family
			AF_NETLINK for netlink sockets
			AF_INET for internet sockets
			AF_UNIX for local communication
			
		SOCK_RAW - socket type (SOCK_RAW for raw sockets)
		NETLINK_USER - protocol type (NETLINK_USER for user-defined protocol)
	Returns:
		sock_fd - file descriptor for the created socket (int)
	*/
    sock_fd = socket(AF_NETLINK,
                     SOCK_RAW,
                     NETLINK_USER);

    if (sock_fd < 0)
    {
        perror("socket");
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));

    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();

    if (bind(sock_fd,
             (struct sockaddr *)&src_addr,
             sizeof(src_addr)) < 0)
    {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));

    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;

    nlh = malloc(NLMSG_SPACE(MAX_PAYLOAD));

    memset(nlh,
           0,
           NLMSG_SPACE(MAX_PAYLOAD));

    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();

	strcpy(NLMSG_DATA(nlh),
       "broadcast");

    iov.iov_base = nlh;
    iov.iov_len = nlh->nlmsg_len;

    memset(&msg,0,sizeof(msg));

    msg.msg_name = &dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov; 
    msg.msg_iovlen = 1; /* Number of elements in the I/O vector */

    printf("Sending : %s\n",
           (char *)NLMSG_DATA(nlh));

    sendmsg(sock_fd,
            &msg,
            0);

    recvmsg(sock_fd,
            &msg,
            0);

    printf("Received : %s\n",
           (char *)NLMSG_DATA(nlh));

    free(nlh);

    close(sock_fd);

    return 0;
}