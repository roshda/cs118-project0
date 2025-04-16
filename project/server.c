#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
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
    if (argc != 2) {
        fprintf(stderr, "Usage: server <port>\n");
        return 1;
    }

    int port = atoi(argv[1]);

    // TODO: create socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // TODO: construct server address
    struct sockaddr_in loc = {0};
    loc.sin_family = AF_INET;
    loc.sin_addr.s_addr = htonl(INADDR_ANY);  // listen on all interfaces
    loc.sin_port = htons(port);

    // TODO: bind address to socket
    if (bind(sock, (struct sockaddr*)&loc, sizeof(loc)) < 0) {
        perror("bind");
        return 1;
    }

    // TODO: set stdin and socket nonblocking
    if (set_nb(STDIN_FILENO) < 0 || set_nb(sock) < 0) {
        perror("fcntl");
        return 1;
    }

    // we’ll use poll() to listen to both stdin and the socket
    struct pollfd pfds[2] = {
        { .fd = STDIN_FILENO, .events = 0 },     // will enable later after client sends first packet
        { .fd = sock,         .events = POLLIN }
    };

    char buf[1024];  // required buffer size
    int stdin_closed = 0;
    int client_ready = 0;

    // TODO: create sockaddr_in and socklen_t buffers to store client address
    struct sockaddr_in client;
    socklen_t clen = sizeof(client);

    // listen loop
    while (1) {
        if (poll(pfds, 2, -1) < 0) {
            perror("poll");
            break;
        }

        // TODO: receive from socket
        if (pfds[1].revents & POLLIN) {
            ssize_t n = recvfrom(sock, buf, sizeof(buf), 0,
                                 (struct sockaddr*)&client, &clen);

            // TODO: if no data and client not connected, continue
            // TODO: if data, client is connected and write to stdout
            if (n > 0) {
                if (!client_ready) {
                    client_ready = 1;
                    pfds[0].events = POLLIN;  // now we care about stdin
                }
                write(STDOUT_FILENO, buf, n);
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recvfrom");
            }
        }

        // TODO: read from stdin
        // TODO: if data, send to socket
        if (client_ready && !stdin_closed &&
            (pfds[0].revents & POLLIN)) {

            ssize_t m = read(STDIN_FILENO, buf, sizeof(buf));
            if (m > 0) {
                sendto(sock, buf, m, 0,
                       (struct sockaddr*)&client, clen);
            } else if (m == 0) {
                // stdin closed (e.g. EOF), stop checking it
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
