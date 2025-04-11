#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define PORT 9035
#define BUFFER_SIZE 2048

const char* http_response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 19\r\n"
    "\r\n"
    "Hello from server!\n";

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) { perror("socket failed"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);
    printf("Target server listening on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) { perror("accept"); continue; }

        int bytes = recv(new_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("Target received request:\n%s\n", buffer);
            send(new_socket, http_response, strlen(http_response), 0);
        }

        close(new_socket);
    }

    return 0;
}
