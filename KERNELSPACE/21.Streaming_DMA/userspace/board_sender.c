/*
 * board_sender.c  -  runs on the STM32MP257F-DK board
 *
 * Sends a data buffer to the PC over the end0 Ethernet link using TCP.
 *
 * Network setup (static IP, as already configured):
 *     Board (end0) : 192.168.2.201/24
 *     PC           : 192.168.2.200/24
 *
 * The PC runs pc_receiver first (it listens), then the board runs this sender.
 *
 * NOTE on DMA:
 *   The actual streaming DMA happens inside the kernel's stmmac Ethernet
 *   driver when this data leaves end0. Userspace only hands the buffer to the
 *   socket; the MAC's DMA engine moves it to the wire.
 *
 * Build (on board, native gcc) :  gcc board_sender.c -o board_sender
 * Cross build (on PC)          :  aarch64-linux-gnu-gcc board_sender.c -o board_sender
 *
 * Run: ./board_sender 192.168.2.200 5001
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DEFAULT_PORT 5001
#define BUF_SIZE     4096

int main(int argc, char *argv[])
{
    const char *server_ip = (argc > 1) ? argv[1] : "192.168.2.200";
    int port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;

    int sock;
    struct sockaddr_in server;
    char buffer[BUF_SIZE];
    ssize_t sent;

    /* Prepare the data buffer (this is the "DMA buffer" equivalent). */
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer),
             "Hello from STM32MP257F-DK over end0 (streaming DMA path)\n");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP: %s\n", server_ip);
        close(sock);
        return 1;
    }

    printf("Connecting to %s:%d ...\n", server_ip, port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("Connected. Sending %zu bytes ...\n", strlen(buffer));

    sent = send(sock, buffer, strlen(buffer), 0);
    if (sent < 0) {
        perror("send");
        close(sock);
        return 1;
    }

    printf("Sent %zd bytes successfully.\n", sent);

    close(sock);
    return 0;
}
