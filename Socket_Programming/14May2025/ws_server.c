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
#define MAX_CLIENTS 10

typedef struct {
    int socket;
    char symbol;
} Client;

Client *clients[MAX_CLIENTS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Game state for one pair
typedef struct {
    Client *playerX;
    Client *playerO;
    char board[9];
} Game;

Game *create_game(Client *x, Client *o) {
    Game *game = malloc(sizeof(Game));
    game->playerX = x;
    game->playerO = o;
    memset(game->board, ' ', 9);
    return game;
}

char *base64_encode(const unsigned char *input, int length) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BUF_MEM *buffer_ptr;
    BIO_get_mem_ptr(b64, &buffer_ptr);
    char *output = (char *)malloc(buffer_ptr->length + 1);
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

void broadcast(Game *game, const char *message) {
    send_ws_message(game->playerX->socket, message);
    send_ws_message(game->playerO->socket, message);
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

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[2048];
    recv(client_socket, buffer, sizeof(buffer), 0);

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
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             encoded);
    free(encoded);
    send(client_socket, response, strlen(response), 0);

    Client *new_client = malloc(sizeof(Client));
    new_client->socket = client_socket;
    new_client->symbol = ' ';

    Client *opponent = NULL;
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = new_client;
            if (i % 2 == 1) opponent = clients[i - 1];
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    if (!opponent) {
        send_ws_message(client_socket, "WAIT");
        while (!opponent) {
            sleep(1);
            pthread_mutex_lock(&lock);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == new_client && i % 2 == 1) {
                    opponent = clients[i - 1];
                    break;
                }
            }
            pthread_mutex_unlock(&lock);
        }
    }

    new_client->symbol = 'O';
    opponent->symbol = 'X';
    Game *game = create_game(opponent, new_client);

    send_ws_message(opponent->socket, "STARTX");
    send_ws_message(new_client->socket, "STARTO");

    Client *players[2] = {opponent, new_client};

    while (1) {
        for (int t = 0; t < 2; t++) {
            char msg[128];
            int n = recv_ws_message(players[t]->socket, msg);
            if (n <= 0) {
                close(players[t]->socket);
                close(players[1 - t]->socket);
                free(game);
                return NULL;
            }

            int pos = atoi(msg) - 1;
            if (pos < 0 || pos > 8 || game->board[pos] != ' ') continue;

            char mark = players[t]->symbol;
            game->board[pos] = mark;

            char update[3] = {mark, msg[0], 0};
            broadcast(game, update);

            int win_patterns[8][3] = {
                {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
                {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
                {0, 4, 8}, {2, 4, 6}
            };
            for (int i = 0; i < 8; i++) {
                if (game->board[win_patterns[i][0]] == mark &&
                    game->board[win_patterns[i][1]] == mark &&
                    game->board[win_patterns[i][2]] == mark) {
                    char win_msg[5] = "WINX";
                    win_msg[3] = mark;
                    broadcast(game, win_msg);
                    close(players[0]->socket);
                    close(players[1]->socket);
                    free(game);
                    return NULL;
                }
            }

            int draw = 1;
            for (int i = 0; i < 9; i++) {
                if (game->board[i] == ' ') draw = 0;
            }
            if (draw) {
                broadcast(game, "DRAW");
                close(players[0]->socket);
                close(players[1]->socket);
                free(game);
                return NULL;
            }
        }
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = INADDR_ANY};
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("Tic Tac Toe server running on ws://localhost:%d\n", PORT);

    while (1) {
        int client_socket = accept(server_fd, NULL, NULL);
        int *pclient = malloc(sizeof(int));
        *pclient = client_socket;
        pthread_t t;
        pthread_create(&t, NULL, handle_client, pclient);
        pthread_detach(t);
    }
}
