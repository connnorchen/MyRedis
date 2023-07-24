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
    // multiple requests
    int32_t err = query(fd, "hello1");
    if (err) {
        goto L_DONE;
    }
    err = query(fd, "hello2");
    if (err) {
        goto L_DONE;
    }
    err = query(fd, "hello3");
    if (err) {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;
}
