#define OPENSSL_API_COMPAT 0x10100000L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdint.h>

#define PORT 8080
#define BUF_SIZE 4096
#define MAX_CLIENTS FD_SETSIZE
#define MAX_PAIRS (MAX_CLIENTS / 2)


char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *encoded = malloc(4 * ((input_length + 2) / 3));
    *output_length = 4 * ((input_length + 2) / 3);

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded[j++] = encoding_table[(triple >> 18) & 0x3F];
        encoded[j++] = encoding_table[(triple >> 12) & 0x3F];
        encoded[j++] = encoding_table[(triple >> 6) & 0x3F];
        encoded[j++] = encoding_table[triple & 0x3F];
    }

    int mod = input_length % 3;
    if (mod > 0) {
        for (int i = 0; i < 3 - mod; i++)
            encoded[*output_length - 1 - i] = '=';
    }

    return encoded;
}

void sha1_hash(const char *input, size_t len, unsigned char *output) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, input, len);
    SHA1_Final(output, &ctx);
}

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


typedef struct {
    char board[9];
    int players[2];
    int turn; // 0 or 1
    int rematch[2];
    int active;
} Game;

Game games[MAX_PAIRS];
int client_pair[MAX_CLIENTS];

void init_game(Game *g, int p1, int p2) {
    for (int i = 0; i < 9; i++) g->board[i] = '1' + i;
    g->players[0] = p1;
    g->players[1] = p2;
    g->turn = 0;
    g->rematch[0] = g->rematch[1] = 0;
    g->active = 1;
}

void send_ws(int fd, const char *msg) {
    uint8_t frame[BUF_SIZE];
    size_t len = strlen(msg);
    frame[0] = 0x81;
    frame[1] = len;
    memcpy(&frame[2], msg, len);
    send(fd, frame, 2 + len, 0);
}

void render_board(char *dst, const char *board) {
    sprintf(dst,
        "%c | %c | %c\n---------\n%c | %c | %c\n---------\n%c | %c | %c",
        board[0], board[1], board[2],
        board[3], board[4], board[5],
        board[6], board[7], board[8]);
}

int check_winner(char *b) {
    int wins[8][3] = {
        {0,1,2},{3,4,5},{6,7,8}, {0,3,6},{1,4,7},{2,5,8}, {0,4,8},{2,4,6}
    };
    for (int i = 0; i < 8; i++) {
        if (b[wins[i][0]] == b[wins[i][1]] && b[wins[i][1]] == b[wins[i][2]])
            return 1;
    }
    return 0;
}

int board_full(char *b) {
    for (int i = 0; i < 9; i++) if (b[i] != 'X' && b[i] != 'O') return 0;
    return 1;
}

void process_move(Game *g, int player_fd, char move_char) {
    int idx = move_char - '1';
    int player_idx = (g->players[0] == player_fd) ? 0 : 1;
    char sym = player_idx == 0 ? 'X' : 'O';

    if (g->players[g->turn] != player_fd) {
        send_ws(player_fd, "Not your turn.");
        return;
    }
    if (idx < 0 || idx > 8 || g->board[idx] == 'X' || g->board[idx] == 'O') {
        send_ws(player_fd, "Invalid move.");
        return;
    }

    g->board[idx] = sym;
    char buf[256];
    render_board(buf, g->board);

    send_ws(g->players[0], buf);
    send_ws(g->players[1], buf);

    if (check_winner(g->board)) {
        sprintf(buf, "Player %c wins! Type 'rematch' to play again.", sym);
        send_ws(g->players[0], buf);
        send_ws(g->players[1], buf);
        g->active = 0;
    } else if (board_full(g->board)) {
        send_ws(g->players[0], "Draw! Type 'rematch' to play again.");
        send_ws(g->players[1], "Draw! Type 'rematch' to play again.");
        g->active = 0;
    } else {
        g->turn = 1 - g->turn;
    }
}

void check_rematch(Game *g) {
    if (g->rematch[0] && g->rematch[1]) {
        init_game(g, g->players[0], g->players[1]);
        char buf[256];
        render_board(buf, g->board);
        send_ws(g->players[0], buf);
        send_ws(g->players[1], buf);
    }
}

int main() {
    int server_fd = create_server_socket();
    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1, client_pair[i] = -1;

    fd_set read_fds;
    printf("WebSocket Tic Tac Toe server on port %d...\n", PORT);

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

            // Pair up
            for (int i = 0; i < MAX_CLIENTS; i += 2) {
                if (clients[i] != -1 && clients[i + 1] != -1 &&
                    client_pair[clients[i]] == -1 && client_pair[clients[i + 1]] == -1) {
                    int pair_idx = i / 2;
                    client_pair[clients[i]] = client_pair[clients[i + 1]] = pair_idx;
                    init_game(&games[pair_idx], clients[i], clients[i + 1]);
                    char buf[256];
                    render_board(buf, games[pair_idx].board);
                    send_ws(clients[i], buf);
                    send_ws(clients[i + 1], buf);
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
                    for (int j = 0; j < payload_len; j++)
                        data[j] ^= mask[j % 4];

                    int game_idx = client_pair[fd];
                    if (game_idx != -1) {
                        Game *g = &games[game_idx];
                        if (strncmp((char *)data, "rematch", 7) == 0) {
                            int pidx = (g->players[0] == fd) ? 0 : 1;
                            g->rematch[pidx] = 1;
                            check_rematch(g);
                        } else if (g->active) {
                            process_move(g, fd, data[0]);
                        }
                    }
                }
            }
        }
    }
}
