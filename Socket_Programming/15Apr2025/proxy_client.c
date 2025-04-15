#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 8192

// Simple connect to proxy or target
int connect_to_server(const char *host, const char *port) {
    struct addrinfo hints, *res, *p;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) break;
        close(sockfd);
    }

    freeaddrinfo(res);
    return (p == NULL) ? -1 : sockfd;
}

void usage(const char *prog) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s -x host:port [URL]\n", prog);
    fprintf(stderr, "  %s -x host:port -X POST -d \"data\" [URL]\n", prog);
    exit(1);
}

int main(int argc, char *argv[]) {
    char *proxy_host = NULL, *proxy_port = NULL;
    char *method = "GET";
    char *data = NULL;
    char *url = NULL;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
            char *colon = strchr(argv[++i], ':');
            if (!colon) usage(argv[0]);
            *colon = '\0';
            proxy_host = argv[i];
            proxy_port = colon + 1;
        } else if (strcmp(argv[i], "-X") == 0 && i + 1 < argc) {
            method = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            data = argv[++i];
        } else {
            url = argv[i];
        }
    }

    if (!proxy_host || !proxy_port || !url) usage(argv[0]);

    int is_https = strncmp(url, "https://", 8) == 0;
    char host[256], path[1024];
    int port = is_https ? 443 : 80;

    // Extract host and path
    if (sscanf(url, "http://%255[^/]%1023s", host, path) < 2) strcpy(path, "/");
    if (is_https) sscanf(url, "https://%255[^/]%1023s", host, path);

    // Check if port is specified in host
    char *colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        port = atoi(colon + 1);
    }

    // Connect to proxy
    int sockfd = connect_to_server(proxy_host, proxy_port);
    if (sockfd < 0) {
        perror("connect_to_proxy");
        exit(1);
    }

    char request[BUFFER_SIZE];
    if (is_https) {
        // Send CONNECT request
        snprintf(request, sizeof(request),
            "CONNECT %s:%d HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "\r\n", host, port, host, port);
        send(sockfd, request, strlen(request), 0);

        char buffer[BUFFER_SIZE];
        int n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
        buffer[n] = '\0';
        printf("Received CONNECT reply:\n%s\n", buffer);
    } else if (strcmp(method, "POST") == 0 && data) {
        // Send POST request
        snprintf(request, sizeof(request),
            "POST http://%s%s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Content-Type: application/x-www-form-urlencoded\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            host, path, host, strlen(data), data);
        send(sockfd, request, strlen(request), 0);
    } else {
        // Send GET request
        snprintf(request, sizeof(request),
            "GET http://%s%s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n"
            "\r\n",
            host, path, host);
        send(sockfd, request, strlen(request), 0);
    }

    // Read and print response
    char response[BUFFER_SIZE];
    int n;
    while ((n = recv(sockfd, response, sizeof(response)-1, 0)) > 0) {
        response[n] = '\0';
        printf("%s", response);
    }

    close(sockfd);
    return 0;
}
