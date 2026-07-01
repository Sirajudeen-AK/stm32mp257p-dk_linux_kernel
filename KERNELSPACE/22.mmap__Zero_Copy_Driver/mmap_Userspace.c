#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main()
{
    int fd;

    char *ptr;

    fd = open("/dev/mmap_demo", O_RDWR);

    if (fd < 0)
    {
        perror("open");
        return -1;
    }

	/*
	API - mmap
		used to map a file or device into memory. 
		It allows user-space applications to access the contents of a file or device as if it were part of the process's memory space, 
		enabling efficient data transfer between user space and kernel space without the need for explicit read/write operations.
	Arguments:
		NULL: The starting address for the mapping (NULL lets the kernel choose).
		4096: The length of the mapping (in bytes).
		PROT_READ | PROT_WRITE: The desired memory protection for the mapping (read and write).
		MAP_SHARED: The mapping is shared with other processes (changes are visible to other processes).
		fd: The file descriptor of the device to be mapped.
		0: The offset within the file/device to start the mapping (0 means start from the beginning).
	Returns:
		A pointer to the mapped memory region on success, or MAP_FAILED on failure. 
	
	*/
    ptr = mmap(NULL,
               4096,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               fd,
               0);

    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return -1;
    }

    printf("Kernel says : %s\n", ptr);

    sprintf(ptr, "Hello from Userspace and address is 0x%p\n", ptr);

    printf("Modified Buffer : %s\n", ptr);

    getchar();

	/* API - munmap
		used to unmap a previously mapped memory region.
		Arguments:
			ptr: The pointer to the mapped memory region.
			4096: The length of the mapping (in bytes).
		Returns:
			0 on success, or -1 on failure.
	*/
    munmap(ptr,4096);

    close(fd);

    return 0;
}