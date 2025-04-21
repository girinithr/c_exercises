#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT "9034"
#define MAX_EVENTS 1024
#define BUFFER_SIZE 8192
#define AUTH_TOKEN "Proxy-Authorization: Basic dXNlcjpwYXNz"

typedef struct {
    int client_fd;
    int upstream_fd;
} connection_t;

connection_t *connections[FD_SETSIZE];

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int connect_to_host(const char *host, int port) {
    struct addrinfo hints, *res, *p;
    int sockfd;
    char port_str[10];
    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    if (getaddrinfo(host, port_str, &hints, &res) != 0) return -1;

    for (p = res; p; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) break;
        close(sockfd);
    }

    freeaddrinfo(res);
    return (p == NULL) ? -1 : sockfd;
}

int check_auth(const char *req) {
    return strstr(req, AUTH_TOKEN) != NULL;
}

int parse_host(const char *req, char *host, int *port) {
    const char *host_hdr = strstr(req, "Host:");
    if (!host_hdr) return -1;
    sscanf(host_hdr, "Host: %255s", host);
    char *colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        *port = atoi(colon + 1);
    } else {
        *port = 80;
    }
    return 0;
}

int get_listener_socket() {
    int sockfd, yes = 1;
    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &ai);
    for (p = ai; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;

        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        break;
    }

    freeaddrinfo(ai);
    if (listen(sockfd, 10) == -1) {
        perror("listen");
        exit(1);
    }

    return sockfd;
}

void relay_data(int from_fd, int to_fd) {
    char buffer[BUFFER_SIZE];
    int len = recv(from_fd, buffer, sizeof(buffer), 0);
    if (len <= 0) return;
    send(to_fd, buffer, len, 0);
}

int main() {
    int listener = get_listener_socket();
    set_nonblocking(listener);

    int epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = listener;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &ev);

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == listener) {
                struct sockaddr_storage client_addr;
                socklen_t addrlen = sizeof client_addr;
                int client_fd = accept(listener, (struct sockaddr *)&client_addr, &addrlen);
                set_nonblocking(client_fd);
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
            } else if (connections[fd]) {
                relay_data(fd, (connections[fd]->client_fd == fd) ? connections[fd]->upstream_fd : connections[fd]->client_fd);
            } else {
                char buf[BUFFER_SIZE] = {0};
                int len = recv(fd, buf, sizeof(buf), 0);
                if (len <= 0) {
                    close(fd);
                    continue;
                }

                if (!check_auth(buf)) {
                    const char *unauth = "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic\r\n\r\n";
                    send(fd, unauth, strlen(unauth), 0);
                    close(fd);
                    continue;
                }

                char host[256];
                int port;
                if (parse_host(buf, host, &port) != 0) {
                    close(fd);
                    continue;
                }

                int upstream_fd = connect_to_host(host, port);
                if (upstream_fd < 0) {
                    close(fd);
                    continue;
                }

                set_nonblocking(upstream_fd);
                ev.events = EPOLLIN;
                ev.data.fd = upstream_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, upstream_fd, &ev);

                connections[fd] = malloc(sizeof(connection_t));
                connections[upstream_fd] = connections[fd];
                connections[fd]->client_fd = fd;
                connections[fd]->upstream_fd = upstream_fd;

                send(upstream_fd, buf, len, 0);
            }
        }
    }

    return 0;
}
