#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENTS 64
#define PORT 8080
#define MAX_CLIENTS 64
#define MAX_GAMES 32
#define BUF_SIZE 1024
#define USERFILE "users.txt"

typedef struct {
    int player1;
    int player2;
    int watchers[MAX_CLIENTS];
    int num_watchers;
    char board[9];
    int turn; // 0 - player1, 1 - player2
} Game;

typedef struct {
    int fd;
    char username[32];
    char mode[16];
    int game_id;
    int paired;
} Client;

Client clients[MAX_CLIENTS];
Game games[MAX_GAMES];
int client_count = 0;
int game_count = 0;

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int find_client(int fd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].fd == fd) return i;
    }
    return -1;
}

int user_exists(const char *username) {
    FILE *f = fopen(USERFILE, "r");
    if (!f) return 0;
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        char *sep = strchr(line, ':');
        if (!sep) continue;
        *sep = '\0';
        if (strcmp(username, line) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int validate_user(const char *username, const char *password) {
    FILE *f = fopen(USERFILE, "r");
    if (!f) return 0;
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        char *sep = strchr(line, ':');
        if (!sep) continue;
        *sep = '\0';
        char *stored_pass = sep + 1;
        stored_pass[strcspn(stored_pass, "\n")] = 0;
        if (strcmp(username, line) == 0 && strcmp(password, stored_pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void register_user(const char *username, const char *password) {
    FILE *f = fopen(USERFILE, "a");
    fprintf(f, "%s:%s\n", username, password);
    fclose(f);
}

void send_client(int fd, const char *msg) {
    send(fd, msg, strlen(msg), 0);
}

void broadcast_game(Game *game, const char *msg) {
    send_client(game->player1, msg);
    send_client(game->player2, msg);
    for (int i = 0; i < game->num_watchers; i++) {
        send_client(game->watchers[i], msg);
    }
}

void handle_login(int client_idx, char *username, char *password) {
    if (user_exists(username)) {
        if (validate_user(username, password)) {
            strcpy(clients[client_idx].username, username);
            send_client(clients[client_idx].fd, "LOGIN_SUCCESS");
        } else {
            send_client(clients[client_idx].fd, "LOGIN_FAIL");
        }
    } else {
        register_user(username, password);
        strcpy(clients[client_idx].username, username);
        send_client(clients[client_idx].fd, "LOGIN_SUCCESS");
    }
}

void create_game(int p1_idx, int p2_idx) {
    Game *g = &games[game_count++];
    g->player1 = clients[p1_idx].fd;
    g->player2 = clients[p2_idx].fd;
    memset(g->board, ' ', 9);
    g->turn = 0;
    g->num_watchers = 0;
    clients[p1_idx].game_id = game_count - 1;
    clients[p2_idx].game_id = game_count - 1;
    clients[p1_idx].paired = 1;
    clients[p2_idx].paired = 1;
    send_client(g->player1, "STARTX");
    send_client(g->player2, "STARTO");
}

void assign_watcher(int client_idx) {
    if (game_count == 0) return;
    int game_id = game_count - 1;
    Game *g = &games[game_id];
    g->watchers[g->num_watchers++] = clients[client_idx].fd;
    clients[client_idx].game_id = game_id;
    send_client(clients[client_idx].fd, "WATCHING");
}

void handle_move(int client_idx, int pos) {
    Client *c = &clients[client_idx];
    Game *g = &games[c->game_id];
    if (pos < 1 || pos > 9 || g->board[pos - 1] != ' ') return;

    char mark = (g->turn == 0) ? 'X' : 'O';
    int expected_fd = (g->turn == 0) ? g->player1 : g->player2;
    if (c->fd != expected_fd) return;

    g->board[pos - 1] = mark;

    char msg[4];
    snprintf(msg, sizeof(msg), "%c%d", mark, pos);
    broadcast_game(g, msg);

    // Check win or draw
    int w[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    for (int i = 0; i < 8; i++) {
        if (g->board[w[i][0]] == mark && g->board[w[i][1]] == mark && g->board[w[i][2]] == mark) {
            char winmsg[8];
            snprintf(winmsg, sizeof(winmsg), "WIN%c", mark);
            broadcast_game(g, winmsg);
            return;
        }
    }
    int draw = 1;
    for (int i = 0; i < 9; i++) if (g->board[i] == ' ') draw = 0;
    if (draw) broadcast_game(g, "DRAW");
    g->turn ^= 1;
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = INADDR_ANY };
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 10);
    set_nonblocking(listen_fd);

    int epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN; ev.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                int client_fd = accept(listen_fd, NULL, NULL);
                set_nonblocking(client_fd);
                ev.events = EPOLLIN; ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                clients[client_count++] = (Client){ .fd = client_fd, .paired = 0 };
            } else {
                char buf[BUF_SIZE];
                int len = recv(fd, buf, sizeof(buf) - 1, 0);
                if (len <= 0) continue;
                buf[len] = 0;
                int idx = find_client(fd);
                if (strncmp(buf, "LOGIN:", 6) == 0) {
                    char *u = strtok(buf + 6, ":");
                    char *p = strtok(NULL, ":");
                    handle_login(idx, u, p);
                } else if (strncmp(buf, "MODE:", 5) == 0) {
                    char *m = buf + 5;
                    strcpy(clients[idx].mode, m);
                    if (strcmp(m, "PLAY") == 0) {
                        for (int j = 0; j < client_count; j++) {
                            if (j != idx && strcmp(clients[j].mode, "PLAY") == 0 && !clients[j].paired) {
                                create_game(j, idx);
                                break;
                            }
                        }
                    } else if (strcmp(m, "WATCH") == 0) {
                        assign_watcher(idx);
                    }
                } else if (isdigit(buf[0])) {
                    int pos = atoi(buf);
                    handle_move(idx, pos);
                }
            }
        }
    }
    return 0;
}

