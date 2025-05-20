#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define MAX_USERS 100
#define MAX_GAMES 50
#define MAX_WATCHERS 10
#define USERNAME_LEN 32
#define PASSWORD_LEN 32
#define BUFFER_SIZE 256

typedef struct {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
} User;

typedef struct {
    struct lws *player_x;
    struct lws *player_o;
    struct lws *watchers[MAX_WATCHERS];
    char board[9];
    int turn; // 0: X, 1: O
    int active;
} Game;

User users[MAX_USERS];
int user_count = 0;
Game games[MAX_GAMES];
int game_count = 0;
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

void load_users() {
    FILE *f = fopen("users.txt", "r");
    if (f) {
        while (fscanf(f, "%s %s", users[user_count].username, users[user_count].password) == 2) {
            user_count++;
        }
        fclose(f);
    }
}

void save_users() {
    FILE *f = fopen("users.txt", "w");
    if (f) {
        for (int i = 0; i < user_count; i++) {
            fprintf(f, "%s %s\n", users[i].username, users[i].password);
        }
        fclose(f);
    }
}

int check_user(const char *u, const char *p) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, u) == 0 && strcmp(users[i].password, p) == 0) return 1;
    }
    return 0;
}

int add_user(const char *u, const char *p) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, u) == 0) return 0;
    }
    if (user_count < MAX_USERS) {
        strncpy(users[user_count].username, u, USERNAME_LEN);
        strncpy(users[user_count].password, p, PASSWORD_LEN);
        user_count++;
        save_users();
        return 1;
    }
    return 0;
}

void send_text(struct lws *wsi, const char *msg) {
    unsigned char buf[LWS_PRE + BUFFER_SIZE];
    size_t len = strlen(msg);
    memcpy(&buf[LWS_PRE], msg, len);
    lws_write(wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
}

int find_or_create_game(struct lws *wsi, int is_player) {
    pthread_mutex_lock(&game_mutex);
    for (int i = 0; i < game_count; i++) {
        if (games[i].player_x && !games[i].player_o) {
            games[i].player_o = wsi;
            pthread_mutex_unlock(&game_mutex);
            return i;
        }
    }
    if (game_count < MAX_GAMES && is_player) {
        Game *g = &games[game_count];
        g->player_x = wsi;
        g->player_o = NULL;
        g->turn = 0;
        memset(g->board, ' ', 9);
        g->active = 1;
        game_count++;
        pthread_mutex_unlock(&game_mutex);
        return game_count - 1;
    }
    for (int i = 0; i < game_count; i++) {
        if (games[i].active) {
            for (int j = 0; j < MAX_WATCHERS; j++) {
                if (!games[i].watchers[j]) {
                    games[i].watchers[j] = wsi;
                    pthread_mutex_unlock(&game_mutex);
                    return i;
                }
            }
        }
    }
    pthread_mutex_unlock(&game_mutex);
    return -1;
}

int check_winner(char *b) {
    int wins[8][3] = {
        {0,1,2},{3,4,5},{6,7,8},
        {0,3,6},{1,4,7},{2,5,8},
        {0,4,8},{2,4,6}
    };
    for (int i = 0; i < 8; i++) {
        if (b[wins[i][0]] != ' ' && b[wins[i][0]] == b[wins[i][1]] && b[wins[i][1]] == b[wins[i][2]])
            return b[wins[i][0]];
    }
    for (int i = 0; i < 9; i++) if (b[i] == ' ') return 0;
    return 'D';
}

int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    static int state = 0;
    static char uname[USERNAME_LEN], passwd[PASSWORD_LEN];
    static int role = 0; // 1 = player, 2 = watcher
    static int game_id = -1;

    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        send_text(wsi, "LOGIN or REGISTER:<username>:<password>");
        break;
    case LWS_CALLBACK_RECEIVE: {
        char *msg = (char *)in;
        if (strncmp(msg, "LOGIN:", 6) == 0 || strncmp(msg, "REGISTER:", 9) == 0) {
            char *u = strtok(msg + (msg[0] == 'L' ? 6 : 9), ":");
            char *p = strtok(NULL, ":");
            if (!u || !p) {
                send_text(wsi, "ERR:Invalid format");
                break;
            }
            strncpy(uname, u, USERNAME_LEN);
            strncpy(passwd, p, PASSWORD_LEN);
            int success = (msg[0] == 'L') ? check_user(uname, passwd) : add_user(uname, passwd);
            if (!success) {
                send_text(wsi, msg[0] == 'L' ? "ERR:Login failed" : "ERR:User exists");
                break;
            }
            send_text(wsi, "OK:Login successful. Send PLAY or WATCH");
        } else if (strcmp(msg, "PLAY") == 0 || strcmp(msg, "WATCH") == 0) {
            role = strcmp(msg, "PLAY") == 0 ? 1 : 2;
            game_id = find_or_create_game(wsi, role == 1);
            if (game_id >= 0) {
                char resp[64];
                snprintf(resp, sizeof(resp), "JOINED:Game %d", game_id);
                send_text(wsi, resp);
            } else {
                send_text(wsi, "ERR:No game slot");
            }
        } else if (msg[0] >= '1' && msg[0] <= '9' && game_id >= 0) {
            Game *g = &games[game_id];
            int pos = msg[0] - '1';
            char mark = (wsi == g->player_x) ? 'X' : (wsi == g->player_o ? 'O' : '?');
            if (mark != '?' && g->board[pos] == ' ') {
                g->board[pos] = mark;
                char state[64];
                snprintf(state, sizeof(state), "%c%d", mark, pos + 1);
                send_text(g->player_x, state);
                send_text(g->player_o, state);
                for (int i = 0; i < MAX_WATCHERS; i++) if (g->watchers[i]) send_text(g->watchers[i], state);
                int winner = check_winner(g->board);
                if (winner) {
                    char end[16];
                    snprintf(end, sizeof(end), winner == 'D' ? "DRAW" : "WIN%c", winner);
                    send_text(g->player_x, end);
                    send_text(g->player_o, end);
                    for (int i = 0; i < MAX_WATCHERS; i++) if (g->watchers[i]) send_text(g->watchers[i], end);
                    g->active = 0;
                }
            }
        }
        break;
    }
    case LWS_CALLBACK_CLOSED:
        // Cleanup omitted for brevity
        break;
    default:
        break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "tictactoe-protocol", callback, 0, BUFFER_SIZE },
    { NULL, NULL, 0, 0 }
};

int main(void) {
    load_users();
    struct lws_context_creation_info info = {0};
    info.port = 8080;
    info.protocols = protocols;
    struct lws_context *context = lws_create_context(&info);
    if (!context) return 1;
    while (1) lws_service(context, 100);
    lws_context_destroy(context);
    return 0;
}
