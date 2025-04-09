#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_BUFFER_SIZE 65535

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <proxy_port> <server_address:server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int proxy_port = atoi(argv[1]);
    char *server_address_port = argv[2];
    char *server_host = strtok(server_address_port, ":"); // <-- renamed
    int server_port = atoi(strtok(NULL, ":"));

    int proxy_fd, client_fd, server_fd;
    struct sockaddr_in proxy_address, client_address, server_address;
    socklen_t client_address_len;
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_received, bytes_sent;

    // Create proxy socket
    proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_fd < 0) {
        perror("Error creating proxy socket");
        exit(EXIT_FAILURE);
    }

    // Configure proxy address
    memset(&proxy_address, 0, sizeof(proxy_address));
    proxy_address.sin_family = AF_INET;
    proxy_address.sin_addr.s_addr = INADDR_ANY;
    proxy_address.sin_port = htons(proxy_port);

    // Bind proxy socket
    if (bind(proxy_fd, (struct sockaddr *)&proxy_address, sizeof(proxy_address)) < 0) {
        perror("Error binding proxy socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(proxy_fd, 5) < 0) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Proxy server listening on port %d...\n", proxy_port);

    while (1) {
        // Accept client connection
        client_address_len = sizeof(client_address);
        client_fd = accept(proxy_fd, (struct sockaddr *)&client_address, &client_address_len);
        if (client_fd < 0) {
            perror("Error accepting client connection");
            continue;
        }

        printf("Accepted client connection.\n");

        // Receive client request
        bytes_received = recv(client_fd, buffer, MAX_BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("Error receiving client request");
            close(client_fd);
            continue;
        }

        buffer[bytes_received] = '\0';
        printf("Received client request:\n%s\n", buffer);

        // Create server socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("Error creating server socket");
            close(client_fd);
            continue;
        }

        // Configure server address
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        struct hostent *host = gethostbyname(server_host); // <-- using correct variable
        if (host == NULL) {
            fprintf(stderr, "Error: Unable to resolve host address: %s\n", server_host);
            close(client_fd);
            close(server_fd);
            continue;
        }
        memcpy(&server_address.sin_addr, host->h_addr_list[0], host->h_length);
        server_address.sin_port = htons(server_port);

        // Connect to server
        if (connect(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            perror("Error connecting to server");
            close(client_fd);
            close(server_fd);
            continue;
        }

        printf("Connected to server.\n");

        // Forward client request to server
        bytes_sent = send(server_fd, buffer, bytes_received, 0);
        if (bytes_sent < 0) {
            perror("Error sending request to server");
            close(client_fd);
            close(server_fd);
            continue;
        }

        // Receive server response and forward to client
        while ((bytes_received = recv(server_fd, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
            bytes_sent = send(client_fd, buffer, bytes_received, 0);
            if (bytes_sent < 0) {
                perror("Error sending response to client");
                break;
            }
        }

        // Close connections
        close(client_fd);
        close(server_fd);

        printf("Closed connections.\n");
    }

    close(proxy_fd);

    return 0;
}
