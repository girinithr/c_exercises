#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <time.h>

#define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 100

SSL_CTX *init_server_ssl_ctx() {
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    if (!SSL_library_init()) {
        fprintf(stderr, "OpenSSL initialization failed\n");
        exit(1);
    }
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "Error loading certificate file\n");
        ERR_print_errors_fp(stderr);
        exit(1);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "Error loading private key file\n");
        ERR_print_errors_fp(stderr);
        exit(1);
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key does not match the certificate public key\n");
        exit(1);
    }
    return ctx;
}

void print_timestamped_message(const char *prefix, const char *msg, int count) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);
    printf("[%s] [%s] Message #%d: %s\n", timebuf, prefix, count, msg);
}

void *handle_client(void *arg) {
    SSL *ssl = (SSL *)arg;
    char buf[MAXDATASIZE];
    int numbytes, msg_count = 0;

    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        pthread_exit(NULL);
    }

    printf("Cipher used: %s\n", SSL_get_cipher(ssl));

    do {
        numbytes = SSL_read(ssl, buf, MAXDATASIZE - 1);
        if (numbytes <= 0) break;
        buf[numbytes] = '\0';
        msg_count++;
        print_timestamped_message("Client", buf, msg_count);

        printf("Enter reply: ");
        fgets(buf, MAXDATASIZE, stdin);
        buf[strcspn(buf, "\n")] = 0;
        SSL_write(ssl, buf, strlen(buf));
    } while (strcmp(buf, "Bye") != 0);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    pthread_exit(NULL);
}

int main(void) {
    SSL_CTX *ctx = init_server_ssl_ctx();
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &servinfo) != 0) {
        perror("getaddrinfo failed");
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) exit(1);
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);
    if (p == NULL) exit(1);
    if (listen(sockfd, BACKLOG) == -1) exit(1);

    printf("SSL server listening on port %s...\n", PORT);

    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) continue;

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, new_fd);

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, ssl);
        pthread_detach(thread);
    }

    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}
