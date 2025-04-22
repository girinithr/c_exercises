#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 8192

void parse_url(const char *url, char *host, char *path, int *port, int *is_https) {
    *is_https = strstr(url, "https://") != NULL;
    const char *prefix = *is_https ? "https://" : "http://";
    const char *start = url + strlen(prefix);

    char *slash = strchr(start, '/');
    if (slash) {
        strncpy(host, start, slash - start);
        host[slash - start] = '\0';
        strcpy(path, slash);
    } else {
        strcpy(host, start);
        strcpy(path, "/");
    }

    *port = *is_https ? 443 : 80;
}

int connect_to_proxy(const char *proxy_host, int proxy_port) {
    struct hostent *server = gethostbyname(proxy_host);
    if (!server) return -1;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {0};

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(proxy_port);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return -1;
    return sockfd;
}

void build_http_request(char *request, const char *method, const char *host, const char *path, const char *data) {
    if (data)
        snprintf(request, BUFFER_SIZE,
                 "%s %s HTTP/1.1\r\nHost: %s\r\nContent-Length: %ld\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\n%s",
                 method, path, host, strlen(data), data);
    else
        snprintf(request, BUFFER_SIZE,
                 "%s %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                 method, path, host);
}

void send_request(int sockfd, const char *request) {
    send(sockfd, request, strlen(request), 0);
    char buffer[BUFFER_SIZE];
    int len;
    while ((len = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[len] = '\0';
        printf("%s", buffer);
    }
    printf("\n");
}

int main() {
    char line[2048];

    while (1) {
        printf("client> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        char method[16], url[1024], data[1024] = "";
        int parts = sscanf(line, "%s %s %[^\n]", method, url, data);

        if (parts < 2) {
            printf("Usage: METHOD URL [data]\n");
            continue;
        }

        char host[256], path[1024];
        int port, is_https;
        parse_url(url, host, path, &port, &is_https);

        if (is_https) {
            printf("HTTPS not supported in this simple client.\n");
            continue;
        }

        int sockfd = connect_to_proxy("127.0.0.1", 9034);
        if (sockfd < 0) {
            perror("connect");
            continue;
        }

        char request[BUFFER_SIZE];
        build_http_request(request, method, host, path, strlen(data) > 0 ? data : NULL);
        send_request(sockfd, request);
        close(sockfd);
    }

    return 0;
}
