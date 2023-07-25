#include <cstring>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include "file_ops.h"
#include "util.h"

static int32_t buffer_req(char buf[], const char *text) {
    size_t len = strlen(text);
    if (len > K_MAX_LENGTH) {
        msg("length is too long", errno);
        return -1;
    }
    memcpy(buf, &len, sizeof(int));
    memcpy(buf + sizeof(int), text, len);
    return 4 + len;
}

static int32_t send_req(int connfd, char buf[], int buf_size) {
    int rv = write_all(connfd, buf, buf_size);
    if (rv) {
        printf("rv %d\n", rv);
        return -1;
    }
    return 0;
}

static int32_t read_res(int connfd, char buf[]) {
    int rv = read_full(connfd, buf, 4);
    if (rv) {
        return -1;
    }
    int len = 0;
    memcpy(&len, buf, sizeof(int));
    if (len > K_MAX_LENGTH) {
        return -1;
    }
    printf("length of msg: %d\n", len);
    
    rv = read_full(connfd, buf + sizeof(int), len);
    if (rv) {
        return -1;
    }
    printf("server says: %.*s\n", len, buf + sizeof(int));
    return 0;
}

static int32_t query(int connfd, const char *text) {
    size_t len = strlen(text);
    if (len > K_MAX_LENGTH) {
        msg("length is too long", errno);
        return -1;
    }
    char write_buf[sizeof(int) + K_MAX_LENGTH] = {};
    memcpy(write_buf, &len, sizeof(int));
    memcpy(write_buf + sizeof(int), text, len);

    int32_t rv = write_all(connfd, write_buf, sizeof(int) + (int) len);
    if (rv) {
        printf("rv %d\n", rv);
        return -1;
    } 

    char req_buf[sizeof(int) + K_MAX_LENGTH] = {}; 
    // | len1 | msg1 format
    rv = read_full(connfd, req_buf, sizeof(int));
    if (rv) {
        return -1;
    }
    len = 0;
    memcpy(&len, req_buf, sizeof(int));
    if (len > K_MAX_LENGTH) {
        return -1;
    }

    
    rv = read_full(connfd, req_buf + sizeof(int), len);
    if (rv) {
        return -1;
    }
    req_buf[sizeof(int) + len] = '\0';
    printf("server says: %s\n", req_buf + sizeof(int));
    return 0;
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()", errno);
    }
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect", errno);
    }
    char buf[K_MAX_LENGTH + 4];
    int32_t len = 0;
    int32_t err = 0;
    for (int i = 0; i < 3; i++) {
        len += buffer_req(&buf[len], "hello1");
        printf("sended %d bytes\n", len);
    }
    send_req(fd, buf, len);
    printf("sended req\n");
    for (int i = 0; i < 3; i++) {
        err = read_res(fd, buf);
        if (err) {
            goto L_DONE;
        }
    }

    // mimik a client hanging and ready to make another request
    while (true) {}
    
L_DONE:
    close(fd);
    return 0;
}
