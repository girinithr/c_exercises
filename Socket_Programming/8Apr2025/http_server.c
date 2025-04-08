#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT "8080"
#define BACKLOG 10
#define MAXDATASIZE 2048

void send_404(int sockfd) {
    const char *not_found =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "Connection: close\r\n"
        "\r\n"
        "404 Not Found";
    send(sockfd, not_found, strlen(not_found), 0);
}

void send_file(int sockfd, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        send_404(sockfd);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    char *html = malloc(fsize + 1);
    fread(html, 1, fsize, fp);
    fclose(fp);
    html[fsize] = '\0';

    char header[256];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n", fsize);

    send(sockfd, header, strlen(header), 0);
    send(sockfd, html, fsize, 0);
    free(html);
}

int main() {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char buf[MAXDATASIZE];
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    listen(sockfd, BACKLOG);
    printf("server: waiting for HTTP connections on port %s...\n", PORT);

    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) continue;

        int numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0);
        if (numbytes > 0) {
            buf[numbytes] = '\0';
            printf("Received request:\n%s\n", buf);

            char method[8], path[256];
            sscanf(buf, "%s %s", method, path);
            if (strcmp(method, "GET") == 0) {
                // Remove leading slash
                char *filename = path[0] == '/' ? path + 1 : path;
                send_file(new_fd, filename);
            } else {
                send_404(new_fd);
            }
        }

        close(new_fd);
    }

    return 0;
}
