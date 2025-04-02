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

#define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 100

SSL_CTX *init_server_ssl_ctx() {
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        perror("SSL_CTX_new failed");
        exit(1);
    }
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        perror("SSL_CTX_use_certificate_file or SSL_CTX_use_PrivateKey_file failed");
        exit(1);
    }
    return ctx;
}

void *handle_client(void *arg) {
    SSL *ssl = (SSL *)arg;
    char buf[MAXDATASIZE];
    int numbytes;

    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        pthread_exit(NULL);
    }

    do {
        numbytes = SSL_read(ssl, buf, MAXDATASIZE - 1);
        if (numbytes <= 0) break;
        buf[numbytes] = '\0';
        printf("Received: %s\n", buf);

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
