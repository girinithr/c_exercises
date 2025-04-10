#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034" // The port client will connect to
#define MAXDATASIZE 256 // Max number of bytes we can get at once

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo);

    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &master);
    FD_SET(STDIN_FILENO, &master);

    for (;;) {
        read_fds = master;
        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            fgets(buf, sizeof buf, stdin);
            if (send(sockfd, buf, strlen(buf), 0) == -1) {
                perror("send");
            }
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            if ((numbytes = recv(sockfd, buf, sizeof buf - 1, 0)) <= 0) {
                if (numbytes == 0) {
                    printf("Server closed connection\n");
                } else {
                    perror("recv");
                }
                close(sockfd);
                exit(0);
            }
            buf[numbytes] = '\0';
            printf("Received: %s", buf);
        }
    }

    return 0;
}
