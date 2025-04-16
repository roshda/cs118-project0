#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// helper to set a file descriptor to non-blocking mode
static int set_nb(int fd) {
    int f = fcntl(fd, F_GETFL, 0);
    return f < 0 ? -1 : fcntl(fd, F_SETFL, f | O_NONBLOCK);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: client <hostname> <port>\n");
        return 1;
    }

    // TODO: resolve hostname to IP
    // if it's literally "localhost", just use 127.0.0.1
    const char* host = strcmp(argv[1], "localhost") == 0 ? "127.0.0.1" : argv[1];
    int port = atoi(argv[2]);

    // TODO: create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // TODO: set stdin and socket non-blocking
    if (set_nb(STDIN_FILENO) < 0 || set_nb(sock) < 0) {
        perror("fcntl");
        return 1;
    }

    // TODO: construct server sockaddr_in
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &srv.sin_addr) != 1) {
        fprintf(stderr, "bad address\n");
        return 1;
    }

    // poll both stdin and socket
    struct pollfd pfds[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = sock,         .events = POLLIN }
    };

    char buf[1024];  // buffer size must be 1024
    int stdin_closed = 0;

    // main loop: send stdin to server, print from server to stdout
    while (1) {
        if (poll(pfds, 2, -1) < 0) {
            perror("poll");
            break;
        }

        // TODO: receive from server
        if (pfds[1].revents & POLLIN) {
            ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
            if (n > 0) {
                // TODO: if data, write to stdout
                write(STDOUT_FILENO, buf, n);
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recvfrom");
            }
        }

        // TODO: read from stdin
        // TODO: if data, send to server
        if (!stdin_closed && (pfds[0].revents & POLLIN)) {
            ssize_t m = read(STDIN_FILENO, buf, sizeof(buf));
            if (m > 0) {
                sendto(sock, buf, m, 0,
                       (struct sockaddr*)&srv, sizeof(srv));
            } else if (m == 0) {
                // EOF — stop checking stdin
                stdin_closed = 1;
                pfds[0].events = 0;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("read");
            }
        }
    }

    close(sock);
    return 0;
}
