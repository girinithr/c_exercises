#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT "8080"
#define MAXDATASIZE 4096

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    if (argc != 2) {
        fprintf(stderr,"Usage: %s <server-hostname>\n", argv[0]);
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
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    // Ask user for filename
    char filename[256];
    printf("Enter the filename to request (e.g., file1.html): ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0;  // remove trailing newline

    // Format HTTP GET request
    char request[512];
    snprintf(request, sizeof(request),
             "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             filename, argv[1]);

    // Send request
    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        close(sockfd);
        exit(1);
    }

    printf("\nHTTP Response:\n\n");

    // Receive response
    while ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) > 0) {
        buf[numbytes] = '\0';
        printf("%s", buf);
    }

    if (numbytes == -1) {
        perror("recv");
    }

    close(sockfd);
    return 0;
}
