/*
 * dma_test.c  -  userspace tester for the /dev/dma_mem2mem driver
 *
 * Writes a string into the driver (which DMA-copies it src->dst inside the
 * kernel using the real DMA controller), then reads it back and prints it.
 *
 * Build (on board) : gcc dma_test.c -o dma_test
 * Run              : ./dma_test "any message you like"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_PATH "/dev/dma_mem2mem"
#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
    const char *msg = (argc > 1) ? argv[1]
                                  : "Hello streaming DMA from userspace!";
    char readback[BUF_SIZE];
    int fd;
    ssize_t n;

    fd = open(DEV_PATH, O_RDWR);
    if (fd < 0) {
        perror("open " DEV_PATH);
        return 1;
    }

    /* Send data into the driver -> triggers a real DMA memcpy in the kernel. */
    n = write(fd, msg, strlen(msg));
    if (n < 0) {
        perror("write");
        close(fd);
        return 1;
    }
    printf("Wrote %zd bytes (DMA-copied inside kernel).\n", n);

    /* Read the DMA-processed result back. */
    lseek(fd, 0, SEEK_SET);
    memset(readback, 0, sizeof(readback));
    n = read(fd, readback, sizeof(readback) - 1);
    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    readback[n] = '\0';

    printf("Read  %zd bytes back: \"%s\"\n", n, readback);

    if (strncmp(msg, readback, strlen(msg)) == 0)
        printf("MATCH - DMA transfer verified.\n");
    else
        printf("MISMATCH - data differs!\n");

    close(fd);
    return 0;
}
