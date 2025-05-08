// main.c
#define OPENSSL_API_COMPAT 0x10100000L  // Suppress OpenSSL 3.0 SHA1 deprecation warnings

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <openssl/sha.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdint.h>

#define PORT 8080
#define BUF_SIZE 4096
#define MAX_CLIENTS FD_SETSIZE

// ---------- BASE64 ----------
char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length) {
    static const char encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *encoded = malloc(4 * ((input_length + 2) / 3));
    *output_length = 4 * ((input_length + 2) / 3);

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        encoded[j++] = encoding_table[(triple >> 18) & 0x3F];
        encoded[j++] = encoding_table[(triple >> 12) & 0x3F];
        encoded[j++] = encoding_table[(triple >> 6) & 0x3F];
        encoded[j++] = encoding_table[triple & 0x3F];
    }

    int mod = input_length % 3;
    if (mod > 0) {
        for (int i = 0; i < 3 - mod; i++) {
            encoded[*output_length - 1 - i] = '=';
        }
    }

    return encoded;
}

// ---------- SHA1 ----------
void sha1_hash(const char *input, size_t len, unsigned char *output) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, input, len);
    SHA1_Final(output, &ctx);
}

// ---------- SERVER ----------
int create_server_socket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);
    return server_fd;
}

void perform_handshake(int client_fd, const char *request) {
    const char *key_header = strstr(request, "Sec-WebSocket-Key:");
    if (!key_header) return;

    char key[128] = {0};
    sscanf(key_header, "Sec-WebSocket-Key: %s", key);

    strcat(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    unsigned char sha1[20];
    sha1_hash(key, strlen(key), sha1);

    size_t encoded_len;
    char *accept_key = base64_encode(sha1, 20, &encoded_len);

    char response[512];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);

    send(client_fd, response, strlen(response), 0);
    free(accept_key);
}

void broadcast(int *clients, int sender, const char *msg, size_t len) {
    uint8_t frame[BUF_SIZE];
    size_t frame_len = 2 + len;
    frame[0] = 0x81;  // FIN + text
    frame[1] = len;

    memcpy(&frame[2], msg, len);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1 && clients[i] != sender) {
            send(clients[i], frame, frame_len, 0);
        }
    }
}

// ---------- MAIN ----------
int main() {
    int server_fd = create_server_socket();
    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    fd_set read_fds;

    printf("WebSocket server listening on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1) {
                FD_SET(clients[i], &read_fds);
                if (clients[i] > max_fd) max_fd = clients[i];
            }
        }

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept(server_fd, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == -1) {
                    clients[i] = client_fd;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = clients[i];
            if (fd != -1 && FD_ISSET(fd, &read_fds)) {
                char buf[BUF_SIZE];
                int len = recv(fd, buf, BUF_SIZE, 0);
                if (len <= 0) {
                    close(fd);
                    clients[i] = -1;
                    continue;
                }

                if (strncmp(buf, "GET", 3) == 0) {
                    perform_handshake(fd, buf);
                } else {
                    uint8_t payload_len = buf[1] & 0x7F;
                    uint8_t *mask = (uint8_t *)&buf[2];
                    uint8_t *data = &buf[6];

                    for (int j = 0; j < payload_len; j++) {
                        data[j] ^= mask[j % 4];
                    }

                    // Print message to server console
                    char msg[BUF_SIZE] = {0};
                    memcpy(msg, data, payload_len);
                    printf("Client %d says: %s\n", fd, msg);

                    broadcast(clients, fd, msg, payload_len);
                }
            }
        }
    }
}
