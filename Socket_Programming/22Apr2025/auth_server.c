#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT "9034"
#define MAX_EVENTS 1024
#define BUFFER_SIZE 8192
#define AUTH_CREDENTIAL "Authorization: Basic YWRtaW46cGFzc3dvcmQ=" // admin:password (base64)

typedef struct {
    int client_fd;
    int upstream_fd;
    int is_https;
} connection_t;

typedef struct CacheEntry {
    char key[1024];
    char response[BUFFER_SIZE];
    struct CacheEntry *next;
} CacheEntry;

connection_t *connections[FD_SETSIZE];
CacheEntry *cache_head = NULL;

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

CacheEntry* find_cache(const char *key) {
    for (CacheEntry *entry = cache_head; entry; entry = entry->next)
        if (strcmp(entry->key, key) == 0) return entry;
    return NULL;
}

void add_cache(const char *key, const char *response) {
    CacheEntry *entry = malloc(sizeof(CacheEntry));
    strncpy(entry->key, key, sizeof(entry->key));
    strncpy(entry->response, response, sizeof(entry->response));
    entry->next = cache_head;
    cache_head = entry;
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

int parse_host(char *request, char *host, int *port, int is_https) {
    char *host_start = strstr(request, "Host:");
    if (!host_start) return -1;
    host_start += 5;
    while (*host_start == ' ') host_start++;
    char *host_end = strstr(host_start, "\r\n");
    if (!host_end) return -1;
    char host_port[512] = {0};
    strncpy(host_port, host_start, host_end - host_start);
    char *colon = strchr(host_port, ':');
    if (colon) {
        *colon = '\0';
        *port = atoi(colon + 1);
    } else {
        *port = is_https ? 443 : 80;
    }
    strcpy(host, host_port);
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

void send_unauthorized(int fd) {
    const char *msg = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Proxy\"\r\nContent-Length: 0\r\n\r\n";
    send(fd, msg, strlen(msg), 0);
}

void handle_connection(int fd, int epoll_fd) {
    char buf[BUFFER_SIZE] = {0};
    int len = recv(fd, buf, sizeof(buf), 0);
    if (len <= 0) {
        close(fd);
        return;
    }

    // Check for basic authentication
    if (!strstr(buf, AUTH_CREDENTIAL)) {
        send_unauthorized(fd);
        close(fd);
        return;
    }

    char method[8], host[256];
    int port;
    sscanf(buf, "%s", method);
    int is_https = strcmp(method, "CONNECT") == 0;

    if (parse_host(buf, host, &port, is_https) != 0) {
        close(fd);
        return;
    }

    // Handle caching for GET
    if (strcmp(method, "GET") == 0) {
        CacheEntry *entry = find_cache(buf);
        if (entry) {
            send(fd, entry->response, strlen(entry->response), 0);
            close(fd);
            return;
        }
    }

    int upstream_fd = connect_to_host(host, port);
    if (upstream_fd < 0) {
        close(fd);
        return;
    }

    set_nonblocking(upstream_fd);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = upstream_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, upstream_fd, &ev);

    connections[fd] = malloc(sizeof(connection_t));
    connections[upstream_fd] = connections[fd];
    connections[fd]->client_fd = fd;
    connections[fd]->upstream_fd = upstream_fd;
    connections[fd]->is_https = is_https;

    if (is_https) {
        send(fd, "HTTP/1.1 200 Connection established\r\n\r\n", 39, 0);
    } else {
        send(upstream_fd, buf, len, 0);
    }
}

void relay_data(int from_fd, int to_fd) {
    char buffer[BUFFER_SIZE];
    int len = recv(from_fd, buffer, sizeof(buffer), 0);
    if (len <= 0) return;

    if (connections[from_fd] && !connections[from_fd]->is_https && to_fd == connections[from_fd]->client_fd) {
        add_cache(buffer, buffer);  // simplistic: cache body as-is
    }

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
                handle_connection(fd, epoll_fd);
            }
        }
    }

    return 0;
}
