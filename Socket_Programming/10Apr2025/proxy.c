// proxy_server.c
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
#define MAX_EVENTS 10
#define BUFFER_SIZE 256

typedef struct {
    int peer_fd;
} FdMap;

FdMap fd_map[FD_SETSIZE] = {0};

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_listener_socket(const char *port) {
    int listener;
    int yes = 1;
    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv = getaddrinfo(NULL, port, &hints, &ai);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) continue;

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai);

    if (!p || listen(listener, 10) == -1) {
        perror("Failed to setup listener");
        exit(2);
    }

    return listener;
}

int connect_to_server(const char *ip, const char *port) {
    struct addrinfo hints, *res, *p;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(ip, port, &hints, &res);
    if (rv != 0) {
        fprintf(stderr, "connect_to_server: %s\n", gai_strerror(rv));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    return (p == NULL) ? -1 : sockfd;
}

int main(void) {
    int listener = get_listener_socket(CLIENT_PORT);
    int epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = listener;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &ev);

    for (;;) {
        int num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < num_fds; i++) {
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

                printf("Proxy connected client %d <-> server %d\n", client_fd, server_fd);

            } else {
                char buf[BUFFER_SIZE];
                int bytes = recv(fd, buf, sizeof buf, 0);
                if (bytes <= 0) {
                    int peer = fd_map[fd].peer_fd;
                    close(fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    if (peer > 0) {
                        close(peer);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, peer, NULL);
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

