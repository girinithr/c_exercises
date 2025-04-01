#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#define PORT 8888
#define MAX_MESSAGE_SIZE 1024

typedef struct {
    struct sockaddr_in serverAddr;
    int socket;
} Client;

Client client;

void* receiveMessages(void* arg) {
    char buffer[MAX_MESSAGE_SIZE];
    ssize_t bytesRead;

    while (1) {
        bytesRead = recvfrom(client.socket, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
        if (bytesRead == -1) {
            perror("Receive failed");
            continue;
        }

        buffer[bytesRead] = '\0';
        printf("%s", buffer);
    }
}

void getTimestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

int main() {
    pthread_t receiveThread;
    char message[MAX_MESSAGE_SIZE];

    if ((client.socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&client.serverAddr, 0, sizeof(client.serverAddr));
    client.serverAddr.sin_family = AF_INET;
    client.serverAddr.sin_port = htons(PORT);
    if (inet_aton("127.0.0.1", &client.serverAddr.sin_addr) == 0) {
        perror("Invalid server address");
        exit(EXIT_FAILURE);
    }

    pthread_create(&receiveThread, NULL, receiveMessages, NULL);

    while (1) {
        char timestamp[64];
        getTimestamp(timestamp, sizeof(timestamp));
        printf("Enter message: ");
        fgets(message, sizeof(message), stdin);
        
        if (strlen(message) == 1) continue;

        char formattedMessage[MAX_MESSAGE_SIZE];
        snprintf(formattedMessage, sizeof(formattedMessage), "[%s] %s", timestamp, message);

        sendto(client.socket, formattedMessage, strlen(formattedMessage), 0, 
               (struct sockaddr*)&client.serverAddr, sizeof(client.serverAddr));
    }

    close(client.socket);
    return 0;
}

