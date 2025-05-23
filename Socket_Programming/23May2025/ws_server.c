#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#define PORT 8080
#define USER_FILE "users.txt"
#define MAX_CLIENTS 10

pthread_mutex_t user_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t game_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int socket;
    char symbol;
} Client;

Client *clients[MAX_CLIENTS];

char *base64_encode(const unsigned char *input, int length) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BUF_MEM *buffer_ptr;
    BIO_get_mem_ptr(b64, &buffer_ptr);
    char *output = malloc(buffer_ptr->length + 1);
    memcpy(output, buffer_ptr->data, buffer_ptr->length);
    output[buffer_ptr->length] = '\0';
    BIO_free_all(b64);
    return output;
}

void send_ws_message(int client_socket, const char *message) {
    size_t len = strlen(message);
    unsigned char frame[2 + 125];
    frame[0] = 0x81;
    frame[1] = len & 0x7F;
    memcpy(&frame[2], message, len);
    send(client_socket, frame, len + 2, 0);
}

int recv_ws_message(int client_socket, char *decoded_msg) {
    unsigned char buffer[128];
    int len = recv(client_socket, buffer, sizeof(buffer), 0);
    if (len <= 0) return -1;

    int payload_len = buffer[1] & 0x7F;
    int mask_offset = 2;
    if (payload_len == 126) mask_offset += 2;
    else if (payload_len == 127) mask_offset += 8;

    unsigned char mask[4];
    memcpy(mask, &buffer[mask_offset], 4);
    int data_start = mask_offset + 4;

    for (int i = 0; i < payload_len; i++) {
        decoded_msg[i] = buffer[data_start + i] ^ mask[i % 4];
    }
    decoded_msg[payload_len] = '\0';
    return payload_len;
}

int user_exists(const char *username) {
    FILE *file = fopen(USER_FILE, "r");
    if (!file) return 0;
    char line[128];
    while (fgets(line, sizeof(line), file)) {
        char *tok = strtok(line, ":");
        if (tok && strcmp(tok, username) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int check_credentials(const char *username, const char *password) {
    FILE *file = fopen(USER_FILE, "r");
    if (!file) return 0;
    char line[128];
    while (fgets(line, sizeof(line), file)) {
        char *user = strtok(line, ":");
        char *pass = strtok(NULL, "\n");
        if (user && pass && strcmp(user, username) == 0 && strcmp(pass, password) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

void register_user(const char *username, const char *password) {
    FILE *file = fopen(USER_FILE, "a");
    if (file) {
        fprintf(file, "%s:%s\n", username, password);
        fclose(file);
    }
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[2048];
    recv(client_socket, buffer, sizeof(buffer), 0);

    // WebSocket handshake
    char *key_header = strstr(buffer, "Sec-WebSocket-Key: ");
    if (!key_header) {
        close(client_socket);
        return NULL;
    }
    key_header += 19;
    char *end = strstr(key_header, "\r\n");
    char key[256] = {0};
    strncpy(key, key_header, end - key_header);

    char combined[256];
    snprintf(combined, sizeof(combined), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)combined, strlen(combined), hash);
    char *encoded = base64_encode(hash, SHA_DIGEST_LENGTH);

    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n", encoded);
    free(encoded);
    send(client_socket, response, strlen(response), 0);

    char msg[512];
    while (1) {
        int n = recv_ws_message(client_socket, msg);
        if (n <= 0) break;

        printf("Received: %s\n", msg);

        char type[16], username[64], password[64];
        pthread_mutex_lock(&user_lock);

        if (sscanf(msg, "{\"type\":\"%15[^\"]\",\"username\":\"%63[^\"]\",\"password\":\"%63[^\"]\"}", type, username, password) == 3) {
            if (strcmp(type, "register") == 0) {
                if (user_exists(username)) {
                    send_ws_message(client_socket, "REGISTER_FAIL");
                } else {
                    register_user(username, password);
                    send_ws_message(client_socket, "REGISTER_SUCCESS");
                }
            } else if (strcmp(type, "login") == 0) {
                if (check_credentials(username, password)) {
                    send_ws_message(client_socket, "LOGIN_SUCCESS");
                } else {
                    send_ws_message(client_socket, "LOGIN_FAIL");
                }
            }
        } else if (sscanf(msg, "{\"type\":\"%15[^\"]\"}", type) == 1) {
            if (strcmp(type, "play") == 0) {
                // Handle play logic
                send_ws_message(client_socket, "WAITING_FOR_OPPONENT");
                // Placeholder for matchmaking logic
            } else if (strcmp(type, "watch") == 0) {
                // Handle watch logic
                send_ws_message(client_socket, "WATCHING_GAMES");
                // Placeholder for spectator logic
            } else {
                send_ws_message(client_socket, "UNKNOWN_COMMAND");
            }
        } else {
            send_ws_message(client_socket, "INVALID_FORMAT");
        }

        pthread_mutex_unlock(&user_lock);
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("WebSocket login/register server running on ws://localhost:%d\n", PORT);

    while (1) {
        int client_socket = accept(server_fd, NULL, NULL);
        int *pclient = malloc(sizeof(int));
        *pclient = client_socket;
        pthread_t t;
        pthread_create(&t, NULL, handle_client, pclient);
        pthread_detach(t);
    }

    return 0;
}
