#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

/*
	Difference between poll and wait queue?
		| Wait Queue            | Poll                    |
		| --------------------- | ----------------------- |
		| Kernel side           | Userspace side          |
		| Driver sleeps process | Application waits on fd |
		| wake_up() wakes it    | poll returns            |


	Interview Questions
		Why poll instead of read?
		
		Without poll:
		read()                     - may block forever.

		With poll:
		poll()                     - application first checks readiness.


	Where is poll used?
		CAN socket
		UART
		Ethernet
		RPMsg
		GPIO interrupt driver
		Video drivers
*/
int main()
{
    int fd;
    char buf[100];

    struct pollfd pfd;

    fd = open("/dev/mychar0", O_RDWR);

	if(fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    pfd.fd = fd;

	/*
		POLLIN means Readable data available
		POLLOUT means Writeable space available
		POLLERR means Error condition
		POLLHUP means Hang up
		POLLNVAL means Invalid request: fd not open
	*/
    pfd.events = POLLIN;

    printf("Waiting...\n");

	/*
	Application calls poll() to check if data is available for reading.
	arguments:
		pfd: pointer to an array of pollfd structures
		1: number of file descriptors to check
		-1: timeout in milliseconds (-1 means wait indefinitely)
	*/
    poll(&pfd, 1, -1);

    printf("Data arrived\n");

    read(fd, buf, sizeof(buf));

    printf("Received: %s\n", buf);

    close(fd);

    return 0;
}