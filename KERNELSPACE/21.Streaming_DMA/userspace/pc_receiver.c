/*
 * pc_receiver.c  -  runs on your PC
 *
 * Listens for a TCP connection from the board (over the direct end0 cable)
 * and prints whatever the board sends.
 *
 * Network setup (static IP, as already configured):
 *     Board (end0) : 192.168.2.201/24
 *     PC           : 192.168.2.200/24
 *
 * Run this FIRST (it waits), then run board_sender on the board.
 *
 * Build: gcc pc_receiver.c -o pc_receiver
 * Run:   ./pc_receiver 5001
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
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;

    int listen_fd, conn_fd;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);
    char buffer[BUF_SIZE];
    ssize_t received;
    int opt = 1;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Allow quick restart without TIME_WAIT errors. */
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;   /* accept on any local interface */
    server.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 1) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("PC receiver listening on port %d ...\n", port);

    conn_fd = accept(listen_fd, (struct sockaddr *)&client, &client_len);
    if (conn_fd < 0) {
        perror("accept");
        close(listen_fd);
        return 1;
    }

    printf("Connection from %s\n", inet_ntoa(client.sin_addr));

    for (;;) {
        received = recv(conn_fd, buffer, sizeof(buffer) - 1, 0);
        if (received < 0) {
            perror("recv");
            break;
        }
        if (received == 0) {
            printf("Board closed the connection.\n");
            break;
        }

        buffer[received] = '\0';
        printf("Received %zd bytes: %s", received, buffer);
    }

    close(conn_fd);
    close(listen_fd);
    return 0;
}
