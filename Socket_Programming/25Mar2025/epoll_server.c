#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>

#define PORT "9034"
#define MAX_EVENTS 10
#define BUFFER_SIZE 256

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
    int listener;
    int yes = 1;
    int rv;
    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "server: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    if (p == NULL) {
        return -1;
    }

    freeaddrinfo(ai);

    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

int main(void)
{
    int listener = get_listener_socket();
    if (listener == -1) {
        fprintf(stderr, "Error getting listening socket\n");
        exit(1);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = listener;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &ev) == -1) {
        perror("epoll_ctl: listener");
        exit(1);
    }

    for (;;) {
        int num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_fds == -1) {
            perror("epoll_wait");
            exit(1);
        }

        for (int i = 0; i < num_fds; i++) {
            if (events[i].data.fd == listener) {
                struct sockaddr_storage remoteaddr;
                socklen_t addrlen = sizeof remoteaddr;
                int newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                if (newfd == -1) {
                    perror("accept");
                } else {
                    ev.events = EPOLLIN;
                    ev.data.fd = newfd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newfd, &ev);
                    printf("New connection on socket %d\n", newfd);
                }
            } else {
                char buf[BUFFER_SIZE];
                int nbytes = recv(events[i].data.fd, buf, sizeof buf, 0);
                if (nbytes <= 0) {
                    if (nbytes == 0) {
                        printf("Socket %d hung up\n", events[i].data.fd);
                    } else {
                        perror("recv");
                    }
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                } else {
                    buf[nbytes] = '\0';
                    printf("Received from client %d: %s\n", events[i].data.fd, buf);
                    send(events[i].data.fd, buf, nbytes, 0);
                }
            }
        }
    }

    close(listener);
    return 0;
}
