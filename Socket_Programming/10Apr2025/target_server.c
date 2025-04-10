#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define PORT 9035
#define BUFFER_SIZE 256
#define MAXDATASIZE 100

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) { perror("socket failed"); exit(1); }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    printf("Target server listening on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) { perror("accept"); continue; }

        //printf("Target: New connection accepted\n");

        while (1) {
            int bytes = recv(new_socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes <= 0) break;

            buffer[bytes] = '\0';
            printf("Target received: %s\n", buffer);

            // Respond back to client through proxy
            char response[BUFFER_SIZE];
            printf("Enter the reply: ");
            fgets(response, MAXDATASIZE, stdin);
            send(new_socket, response, strlen(response), 0);
        }

        printf("Target: Connection closed\n");
        close(new_socket);
    }

    return 0;
}

