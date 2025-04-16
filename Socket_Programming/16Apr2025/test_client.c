#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CONNECTIONS 10000
#define PROXY_IP "127.0.0.1"
#define PROXY_PORT 9034
#define HTTP_REQUEST "GET http://localhost:8080/ HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"

int create_connection() {
    int sock;
    struct sockaddr_in proxy_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(PROXY_PORT);
    inet_pton(AF_INET, PROXY_IP, &proxy_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        close(sock);
        return -1;
    }

    send(sock, HTTP_REQUEST, strlen(HTTP_REQUEST), 0);
    return sock;
}

int main() {
    int sockets[MAX_CONNECTIONS];
    int count = 0;

    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        int sock = create_connection();
        if (sock == -1) {
            printf("Failed at %d connections\n", i);
            break;
        }
        sockets[i] = sock;
        if (i % 100 == 0) printf("Opened %d connections\n", i);
        count++;
    }

    printf("Total successful connections: %d\n", count);

    for (int i = 0; i < count; ++i) {
        char buffer[1024];
        recv(sockets[i], buffer, sizeof(buffer), 0);  // Optionally read response
        close(sockets[i]);
    }

    return 0;
}
