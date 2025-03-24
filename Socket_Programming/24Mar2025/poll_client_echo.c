#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define SERVER_IP "127.0.0.1" // Change this if needed
#define PORT 9034
#define BUFFER_SIZE 256

void add_to_pfds(struct pollfd **pfds, int *fd_count, int *fd_size, int newfd) {
    if (*fd_count == *fd_size) {
        *fd_size *= 2;
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }
    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;
    (*fd_count)++;
}

void del_from_pfds(struct pollfd *pfds, int i, int *fd_count) {
    pfds[i] = pfds[*fd_count - 1];
    (*fd_count)--;
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server. Type messages and press Enter to send.\n");

    int fd_count = 0;
    int fd_size = 2;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    add_to_pfds(&pfds, &fd_count, &fd_size, sockfd);
    add_to_pfds(&pfds, &fd_count, &fd_size, STDIN_FILENO);

    for (;;) {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        for (int i = 0; i < fd_count; i++) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == sockfd) {
                    int nbytes = recv(sockfd, buf, sizeof(buf) - 1, 0);
                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                            printf("Server disconnected.\n");
                        } else {
                            perror("recv");
                        }
                        close(sockfd);
                        del_from_pfds(pfds, i, &fd_count);
                        free(pfds);
                        return 0;
                    }
                    buf[nbytes] = '\0';
                    printf("Server: %s", buf);
                } else if (pfds[i].fd == STDIN_FILENO) {
                    if (fgets(buf, sizeof(buf), stdin) == NULL) {
                        break;
                    }
                    send(sockfd, buf, strlen(buf), 0);
                }
            }
        }
    }

    free(pfds);
    close(sockfd);
    return 0;
}
