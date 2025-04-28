// websocket_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 4096

void websocket_send(int sockfd, const char *message) {
    uint8_t frame[10];
    size_t len = strlen(message);
    size_t frame_size = 0;
    uint8_t mask_key[4];

    frame[0] = 0x81; // FIN + Text frame opcode

    srand(time(NULL));
    for (int i = 0; i < 4; i++) {
        mask_key[i] = rand() % 256;
    }

    if (len <= 125) {
        frame[1] = 0x80 | (uint8_t)len; // MASK bit set + payload length
        frame_size = 2;
    } else if (len <= 65535) {
        frame[1] = 0x80 | 126;
        frame[2] = (len >> 8) & 0xFF;
        frame[3] = len & 0xFF;
        frame_size = 4;
    } else {
        frame[1] = 0x80 | 127;
        frame[2] = (len >> 56) & 0xFF;
        frame[3] = (len >> 48) & 0xFF;
        frame[4] = (len >> 40) & 0xFF;
        frame[5] = (len >> 32) & 0xFF;
        frame[6] = (len >> 24) & 0xFF;
        frame[7] = (len >> 16) & 0xFF;
        frame[8] = (len >> 8) & 0xFF;
        frame[9] = len & 0xFF;
        frame_size = 10;
    }

    // Send frame header
    send(sockfd, frame, frame_size, 0);

    // Send mask key
    send(sockfd, mask_key, 4, 0);

    // Mask the payload
    uint8_t *masked = malloc(len);
    for (size_t i = 0; i < len; i++) {
        masked[i] = message[i] ^ mask_key[i % 4];
    }

    // Send masked payload
    send(sockfd, masked, len, 0);

    free(masked);
}

void perform_handshake(int sockfd) {
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request),
        "GET /chat HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        SERVER_IP, SERVER_PORT);

    send(sockfd, request, strlen(request), 0);

    char response[BUFFER_SIZE];
    recv(sockfd, response, sizeof(response) - 1, 0);
    // We don't fully verify server handshake here for simplicity
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to WebSocket server %s:%d\n", SERVER_IP, SERVER_PORT);

    perform_handshake(sockfd);
    printf("Handshake completed. You can now send messages.\n");

    while (1) {
        printf("Enter message (type 'exit' to quit): ");
        if (!fgets(message, sizeof(message), stdin)) {
            break;
        }
        size_t len = strlen(message);
        if (message[len - 1] == '\n') message[len - 1] = '\0'; // Remove newline

        if (strcmp(message, "exit") == 0) {
            break;
        }

        websocket_send(sockfd, message);
    }

    close(sockfd);
    printf("Disconnected.\n");
    return 0;
}
