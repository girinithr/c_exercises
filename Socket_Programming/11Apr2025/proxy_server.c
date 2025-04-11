#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>

#define CLIENT_PORT "9034"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "9035"
#define MAX_EVENTS 64
#define BUFFER_SIZE 2048

typedef struct {
    int peer_fd;
} FdMap;

FdMap fd_map[FD_SETSIZE];

int get_listener_socket(const char *port) {
    int listener;
    struct addrinfo hints, *res, *p;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    for (p = res; p; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener == -1) continue;

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            close(listener);
            continue;
        }

        break;
    }

    if (!p) {
        fprintf(stderr, "Failed to bind\n");
        exit(2);
    }

    freeaddrinfo(res);

    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    return listener;
}

int connect_to_server(const char *ip, const char *port) {
    struct addrinfo hints, *res, *p;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(ip, port, &hints, &res);

    for (p = res; p; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(res);
    return p ? sockfd : -1;
}

int main(void) {
    int listener = get_listener_socket(CLIENT_PORT);
    int epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = listener;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &ev);

    printf("Proxy server listening on port %s\n", CLIENT_PORT);

    for (;;) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == listener) {
                struct sockaddr_storage client_addr;
                socklen_t addrlen = sizeof client_addr;
                int client_fd = accept(listener, (struct sockaddr *)&client_addr, &addrlen);
                if (client_fd == -1) continue;

                int server_fd = connect_to_server(SERVER_IP, SERVER_PORT);
                if (server_fd == -1) {
                    close(client_fd);
                    continue;
                }

                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

                ev.data.fd = server_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

                fd_map[client_fd].peer_fd = server_fd;
                fd_map[server_fd].peer_fd = client_fd;

                printf("Proxy: Connected client %d <-> server %d\n", client_fd, server_fd);

            } else {
                char buf[BUFFER_SIZE];
                int bytes = recv(fd, buf, sizeof buf, 0);
                if (bytes <= 0) {
                    int peer = fd_map[fd].peer_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    if (peer > 0) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, peer, NULL);
                        close(peer);
                    }
                    continue;
                }

                int peer_fd = fd_map[fd].peer_fd;
                send(peer_fd, buf, bytes, 0);
            }
        }
    }

    close(listener);
    return 0;
}
