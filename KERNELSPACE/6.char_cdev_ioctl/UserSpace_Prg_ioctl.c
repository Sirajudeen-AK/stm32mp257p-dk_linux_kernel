#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct my_cfg {
    int id;
    int speed;
    char name[32];
}my_cfg = {7, 100, "My Name is siraj"};

#define MY_IOCTL_MAGIC 'A'

/*
Arguments:
	1. Magic number
	2. Command number
	3. Type of data to be passed (int/char/struct)
*/
#define SET_VALUE _IOW(MY_IOCTL_MAGIC, 1, struct my_cfg )
#define GET_VALUE _IOR(MY_IOCTL_MAGIC, 2, struct my_cfg )

int main()
{
    int fd;

    fd = open("/dev/mychar0", O_RDWR);
    if (fd < 0) {
		perror("Failed to open device");
		return -1;
	}
	
    ioctl(fd, SET_VALUE, &my_cfg);

    ioctl(fd, GET_VALUE, &my_cfg);

    printf("Kernel value = id: %d, speed: %d, name: %s\n", my_cfg.id, my_cfg.speed, my_cfg.name);

    close(fd);

    return 0;
}