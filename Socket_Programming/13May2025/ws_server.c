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
    int turn; // 0 for X, 1 for O
} Game;

Game *create_game(Client *x, Client *o) {
    Game *game = malloc(sizeof(Game));
    game->playerX = x;
    game->playerO = o;
    memset(game->board, ' ', 9);
    game->turn = 0;
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

    char accept_key[256];
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
            if (i % 2 == 1) {
                opponent = clients[i - 1];
            }
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    if (opponent == NULL) {
        send_ws_message(client_socket, "WAIT");
        while (opponent == NULL) {
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

    while (1) {
        unsigned char msg[128];
        int len = recv(client_socket, msg, sizeof(msg), 0);
        if (len <= 0) break;

        int payload_len = msg[1] & 0x7F;
        int mask_idx = 2;
        unsigned char decoded = ((msg[mask_idx + 0] ^ msg[mask_idx + 4]));

        int pos = decoded - '1';
        if (pos < 0 || pos > 8 || game->board[pos] != ' ')
            continue;

        char mark = new_client->symbol;
        game->board[pos] = mark;

        char update[3] = {mark, decoded, 0};
        broadcast(game, update);

        // Check for win
        int wins[8][3] = {
            {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
            {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
            {0, 4, 8}, {2, 4, 6}
        };
        for (int i = 0; i < 8; i++) {
            if (game->board[wins[i][0]] == mark &&
                game->board[wins[i][1]] == mark &&
                game->board[wins[i][2]] == mark) {
                char win_msg[5] = "WINX";
                win_msg[3] = mark;
                broadcast(game, win_msg);
                close(game->playerX->socket);
                close(game->playerO->socket);
                free(game);
                return NULL;
            }
        }

        // Check for draw
        int full = 1;
        for (int i = 0; i < 9; i++) {
            if (game->board[i] == ' ') {
                full = 0;
                break;
            }
        }
        if (full) {
            broadcast(game, "DRAW");
            close(game->playerX->socket);
            close(game->playerO->socket);
            free(game);
            return NULL;
        }
    }

    close(client_socket);
    return NULL;
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
