#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <time.h>

#define PORT "3490"
#define MAXDATASIZE 100

SSL_CTX *init_client_ssl_ctx() {
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        perror("SSL_CTX_new failed");
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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(1);
    }

    SSL_CTX *ctx = init_client_ssl_ctx();
    int sockfd;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(argv[1], PORT, &hints, &servinfo) != 0) {
        perror("getaddrinfo failed");
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) exit(1);
    freeaddrinfo(servinfo);

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sockfd);
        exit(1);
    }

    printf("Connected to server with SSL encryption.\n");
    char buf[MAXDATASIZE];
    int numbytes, msg_count = 0;

    do {
        printf("Enter message: ");
        fgets(buf, MAXDATASIZE, stdin);
        buf[strcspn(buf, "\n")] = 0;
        msg_count++;
        SSL_write(ssl, buf, strlen(buf));
        print_timestamped_message("Client -> Server", buf, msg_count);

        numbytes = SSL_read(ssl, buf, MAXDATASIZE - 1);
        if (numbytes <= 0) break;
        buf[numbytes] = '\0';
        print_timestamped_message("Server -> Client", buf, msg_count);
    } while (strcmp(buf, "Bye") != 0);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    return 0;
}
