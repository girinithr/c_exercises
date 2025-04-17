#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_INPUT 2048
#define BUFFER_SIZE 8192

// Extract hostname and path from URL
void parse_url(const char *url, char *host, char *path) {
    sscanf(url, "http://%255[^/]%1023s", host, path);
    if (strlen(path) == 0) strcpy(path, "/");
}

// Connect to proxy host:port
int connect_to_proxy(const char *proxy_host, const char *proxy_port) {
    struct addrinfo hints, *res, *p;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(proxy_host, proxy_port, &hints, &res) != 0) {
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

// Send HTTP request through proxy
void send_http_request(const char *method, const char *host, const char *path, const char *data, int sockfd) {
    char request[BUFFER_SIZE];

    if (strcasecmp(method, "POST") == 0 || strcasecmp(method, "PUT") == 0) {
        snprintf(request, sizeof(request),
                 "%s http://%s%s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "Content-Type: application/x-www-form-urlencoded\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "%s",
                 method, host, path, host, strlen(data), data);
    } else {
        snprintf(request, sizeof(request),
                 "%s http://%s%s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 method, host, path, host);
    }

    send(sockfd, request, strlen(request), 0);
}

// Read and print response
void print_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    int len;

    while ((len = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[len] = '\0';
        printf("%s", buffer);
    }
}

int main() {
    char input[MAX_INPUT];
    printf("Interactive HTTP Client. Type your commands like curl:\n");

    while (1) {
        printf("httpc> ");
        if (!fgets(input, sizeof(input), stdin)) break;

        if (strncmp(input, "exit", 4) == 0) break;

        // Parse the input
        char *method = "GET";
        char *proxy = NULL;
        char *url = NULL;
        char *data = NULL;

        char *token = strtok(input, " \t\n");
        while (token) {
            if (strcmp(token, "-x") == 0) {
                proxy = strtok(NULL, " \t\n");
            } else if (strcmp(token, "-X") == 0) {
                method = strtok(NULL, " \t\n");
            } else if (strcmp(token, "-d") == 0) {
                data = strtok(NULL, " \t\n");
            } else if (strncmp(token, "http://", 7) == 0) {
                url = token;
            }
            token = strtok(NULL, " \t\n");
        }

        if (!proxy || !url) {
            printf("Usage: -x <proxy> <url> [-X <METHOD>] [-d <data>]\n");
            continue;
        }

        // Split proxy into host and port
        char proxy_host[256], proxy_port[10] = "80";
        if (sscanf(proxy, "http://%255[^:]:%9s", proxy_host, proxy_port) < 2) {
            sscanf(proxy, "http://%255s", proxy_host);
        }

        char host[256], path[1024] = "/";
        parse_url(url, host, path);

        int sockfd = connect_to_proxy(proxy_host, proxy_port);
        if (sockfd < 0) {
            perror("connect_to_proxy");
            continue;
        }

        send_http_request(method, host, path, data ? data : "", sockfd);
        print_response(sockfd);
        close(sockfd);
        printf("\n");
    }

    return 0;
}

