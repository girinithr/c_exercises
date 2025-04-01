#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#define PORT 8888
#define MAX_MESSAGE_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    struct sockaddr_in address;
    int socket;
    int id;
    int message_count;
} Client;

Client clients[MAX_CLIENTS];
pthread_t threads[MAX_CLIENTS];
int clientCount = 0;
pthread_mutex_t mutex;

void getTimestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

void *handleClient(void *arg) {
    int id = *((int *)arg);
    char buffer[MAX_MESSAGE_SIZE];
    ssize_t bytesRead;
    socklen_t addrLen = sizeof(clients[id].address);

    while (1) {
        bytesRead = recvfrom(clients[id].socket, buffer, sizeof(buffer) - 1, 0, 
                             (struct sockaddr*)&clients[id].address, &addrLen);
        if (bytesRead == -1) {
            perror("Receive failed");
            pthread_exit(NULL);
        }

        buffer[bytesRead] = '\0';
        pthread_mutex_lock(&mutex);
        clients[id].message_count++;
        pthread_mutex_unlock(&mutex);

        char timestamp[64];
        getTimestamp(timestamp, sizeof(timestamp));

        printf("[%s] Client %d (Msg %d): %s", timestamp, id + 1, clients[id].message_count, buffer);

        // Send acknowledgment with message count
        snprintf(buffer, sizeof(buffer), "[%s] Server: Acknowledged Message %d from Client %d\n", 
                 timestamp, clients[id].message_count, id + 1);
        sendto(clients[id].socket, buffer, strlen(buffer), 0, 
               (struct sockaddr*)&clients[id].address, addrLen);
    }
    return NULL;
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[MAX_MESSAGE_SIZE];
    socklen_t clientLen = sizeof(clientAddr);
    ssize_t bytesRead;

    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&mutex, NULL);
    printf("Server listening on port %d...\n", PORT);

    while (clientCount < MAX_CLIENTS) {
        bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer) - 1, 0, 
                             (struct sockaddr*)&clientAddr, &clientLen);
        if (bytesRead == -1) {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }
        buffer[bytesRead] = '\0';

        pthread_mutex_lock(&mutex);
        clients[clientCount].socket = serverSocket;
        clients[clientCount].address = clientAddr;
        clients[clientCount].id = clientCount;
        clients[clientCount].message_count = 0;
        pthread_create(&threads[clientCount], NULL, handleClient, &clients[clientCount].id);
        clientCount++;
        pthread_mutex_unlock(&mutex);
    }

    close(serverSocket);
    pthread_mutex_destroy(&mutex);
    return 0;
}

