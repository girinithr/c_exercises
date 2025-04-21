#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 8192

void parse_url(const char *url, char *host, char *path, int *port) {
    sscanf(url, "http://%255[^/]%1023s", host, path);
    if (strlen(path) == 0) strcpy(path, "/");
    *port = 80;
}

int connect_to_proxy(const char *host, int port) {
    struct sockaddr_in servaddr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, host, &servaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) return -1;
    return sockfd;
}

void build_http_request(char *request, const char *method, const char *host, const char *path, const char *data, int auth) {
    const char *auth_hdr = auth ? "Proxy-Authorization: Basic dXNlcjpwYXNz\r\n" : "";
    if (data)
        snprintf(request, BUFFER_SIZE,
                 "%s %s HTTP/1.1\r\nHost: %s\r\n%sContent-Length: %ld\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\n%s",
                 method, path, host, auth_hdr, strlen(data), data);
    else
        snprintf(request, BUFFER_SIZE,
                 "%s %s HTTP/1.1\r\nHost: %s\r\n%sConnection: close\r\n\r\n",
                 method, path, host, auth_hdr);
}

void send_request(int sockfd, const char *request) {
    char buffer[BUFFER_SIZE];
    send(sockfd, request, strlen(request), 0);
    int n;
    while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }
}

int main() {
    char line[2048];

    while (1) {
        printf("client> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        char method[16] = "GET", url[1024], data[1024] = "";
        int use_auth = strstr(line, "--auth") != NULL;

        sscanf(line, "proxy %s -X %s -d %[^\n]", url, method, data);
        if (!strstr(line, "-X")) strcpy(method, "GET");
        if (!strstr(line, "-d")) data[0] = '\0';

        char host[256], path[1024];
        int port;
        parse_url(url, host, path, &port);

        int sockfd = connect_to_proxy("127.0.0.1", 9034);
        if (sockfd < 0) continue;

        char request[BUFFER_SIZE];
        build_http_request(request, method, host, path, strlen(data) ? data : NULL, use_auth);
        send_request(sockfd, request);
        close(sockfd);
        printf("\n");
    }

    return 0;
}
