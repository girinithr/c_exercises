/*
** multithreaded_server.c -- a multithreaded stream socket server demo with timestamp
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 100

typedef struct {
    int socket;
    struct sockaddr_storage client_addr;
} client_info;

void get_timestamp(char *buffer, size_t size) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

void *handle_client(void *arg) {
    client_info *cinfo = (client_info *)arg;
    int new_fd = cinfo->socket;
    char buf[MAXDATASIZE];
    char s[INET6_ADDRSTRLEN];
    int numbytes, msg_count = 0;

    inet_ntop(AF_INET, &(((struct sockaddr_in*)&cinfo->client_addr)->sin_addr), s, sizeof s);
    printf("server: got connection from %s\n", s);

    do {
        if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1) {
            perror("recv");
            break;
        }
        buf[numbytes] = '\0';
        printf("Message %d: received '%s'\n", ++msg_count, buf);

        printf("Enter a string: ");
        fgets(buf, MAXDATASIZE, stdin);
        buf[strcspn(buf, "\n")] = 0;

        char timestamp[20];
        get_timestamp(timestamp, sizeof(timestamp));
        char message[MAXDATASIZE + 50];
        snprintf(message, sizeof(message), "[%s] %s", timestamp, buf);

        if (send(new_fd, message, strlen(message), 0) == -1) {
            perror("send");
            break;
        }
    } while (strcmp(buf, "Bye") != 0);

    close(new_fd);
    free(cinfo);
    pthread_exit(NULL);
}

int main(void) {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        client_info *cinfo = malloc(sizeof(client_info));
        if (!cinfo) {
            perror("malloc");
            close(new_fd);
            continue;
        }
        cinfo->socket = new_fd;
        cinfo->client_addr = their_addr;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, cinfo) != 0) {
            perror("pthread_create");
            close(new_fd);
            free(cinfo);
            continue;
        }

        pthread_detach(thread);
    }

    return 0;
}
