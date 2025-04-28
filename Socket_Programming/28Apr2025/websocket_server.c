#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <openssl/sha.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ctype.h>

#define PORT 8080
#define MAX_EVENTS 64
#define MAX_CLIENTS 128
#define BUF_SIZE 4096

typedef struct {
    int fd;
    int paired_fd;
    int handshake_done;
} client_t;

client_t *clients[MAX_CLIENTS];

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void remove_client(int fd, int epoll_fd) {
    if (clients[fd]) {
        printf("Closing connection %d\n", fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        if (clients[fd]->paired_fd != -1 && clients[clients[fd]->paired_fd]) {
            clients[clients[fd]->paired_fd]->paired_fd = -1;
        }
        free(clients[fd]);
        clients[fd] = NULL;
    }
}

void send_websocket_handshake(int client_fd, char *request) {
    char *key_start = strstr(request, "Sec-WebSocket-Key: ");
    if (!key_start) return;
    key_start += 19; // Move past the header

    char *key_end = strstr(key_start, "\r\n");
    if (!key_end) return;

    char client_key[256] = {0};
    strncpy(client_key, key_start, key_end - key_start);

    const char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char concatenated[512];
    snprintf(concatenated, sizeof(concatenated), "%s%s", client_key, guid);

    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)concatenated, strlen(concatenated), sha1_result);

    char base64_result[512];
    int encoded_len = EVP_EncodeBlock((unsigned char *)base64_result, sha1_result, SHA_DIGEST_LENGTH);

    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n", base64_result);

    send(client_fd, response, strlen(response), 0);
}

void websocket_send(int fd, const uint8_t *data, size_t len) {
    uint8_t frame[10];
    size_t frame_size = 0;
    frame[0] = 0x81; // FIN + text frame
    if (len <= 125) {
        frame[1] = len;
        frame_size = 2;
    } else if (len <= 65535) {
        frame[1] = 126;
        frame[2] = (len >> 8) & 0xFF;
        frame[3] = len & 0xFF;
        frame_size = 4;
    } else {
        frame[1] = 127;
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

    send(fd, frame, frame_size, 0);
    send(fd, data, len, 0);
}

int websocket_read(int fd, uint8_t *buffer, size_t len) {
    uint8_t header[2];
    int r = recv(fd, header, 2, 0);
    if (r <= 0) return -1;

    int payload_len = header[1] & 0x7F;
    if (payload_len == 126) {
        uint8_t ext[2];
        recv(fd, ext, 2, 0);
        payload_len = (ext[0] << 8) | ext[1];
    } else if (payload_len == 127) {
        uint8_t ext[8];
        recv(fd, ext, 8, 0);
        payload_len = (ext[6] << 8) | ext[7];
    }

    uint8_t mask[4];
    recv(fd, mask, 4, 0);

    if (payload_len > len) return -1;

    r = recv(fd, buffer, payload_len, 0);
    if (r <= 0) return -1;

    for (int i = 0; i < payload_len; i++) {
        buffer[i] ^= mask[i % 4];
    }

    return payload_len;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblocking(server_fd);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 128);

    int epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    printf("WebSocket server running on port %d\n", PORT);

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                // New connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                set_nonblocking(client_fd);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

                clients[client_fd] = calloc(1, sizeof(client_t));
                clients[client_fd]->fd = client_fd;
                clients[client_fd]->paired_fd = -1;

                printf("New client connected: %d\n", client_fd);

                // Try pairing
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j] && clients[j]->paired_fd == -1 && j != client_fd) {
                        clients[client_fd]->paired_fd = j;
                        clients[j]->paired_fd = client_fd;
                        printf("Paired %d with %d\n", client_fd, j);
                        break;
                    }
                }
            } else {
                int fd = events[i].data.fd;
                if (!clients[fd]) continue;

                if (!clients[fd]->handshake_done) {
                    // Read HTTP handshake
                    char buf[BUF_SIZE];
                    int r = recv(fd, buf, sizeof(buf)-1, 0);
                    if (r <= 0) {
                        remove_client(fd, epoll_fd);
                        continue;
                    }
                    buf[r] = '\0';
                    if (strstr(buf, "\r\n\r\n")) {
                        send_websocket_handshake(fd, buf);
                        clients[fd]->handshake_done = 1;
                        printf("Handshake completed with %d\n", fd);
                    }
                } else {
                    // WebSocket data
                    uint8_t buffer[BUF_SIZE];
                    int r = websocket_read(fd, buffer, sizeof(buffer));
                    if (r <= 0) {
                        remove_client(fd, epoll_fd);
                    } else {
                        int peer_fd = clients[fd]->paired_fd;
                        if (peer_fd != -1 && clients[peer_fd]) {
                            websocket_send(peer_fd, buffer, r);
                        }
                    }
                }
            }
        }
    }
}
