#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9034
#define BUFFER_SIZE 256

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) { perror("socket"); exit(1); }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect"); close(sockfd); exit(1);
    }

    printf("Connected to proxy server. Start chatting.\n");

    while (1) {
        printf("You: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (send(sockfd, buffer, strlen(buffer), 0) == -1) {
            perror("send"); break;
        }

        int bytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            if (bytes == 0)
                printf("Disconnected.\n");
            else
                perror("recv");
            break;
        }

        buffer[bytes] = '\0';
        printf("Server: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}

