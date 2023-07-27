#include <_types/_uint32_t.h>
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
#include <vector>
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

static int32_t send_req(int connfd, std::vector<std::string> &cmd) {
    uint32_t len = 4;

    for (const std::string &s : cmd) {
        len += 4 + ((uint32_t) s.size());
    }
    if (len > K_MAX_LENGTH) {
        msg("req length is too long", -1);
        return -1;
    }

    char send_buf[4 + K_MAX_LENGTH];
    uint32_t sz = (uint32_t) cmd.size();
    memcpy(send_buf, &len, 4);
    memcpy(&send_buf[4], &sz, 4);
    
    size_t pos = 8;
    for (const std::string &s: cmd) {
        uint32_t str_len = (uint32_t) s.size();
        memcpy(&send_buf[pos], &str_len, 4);
        memcpy(&send_buf[pos + 4], s.data(), s.size());
        pos += 4 + s.size(); 
    }
    return write_all(connfd, send_buf, len + 4);
}

static int32_t read_res(int connfd) {
    char rbuf[4 + K_MAX_LENGTH];
    uint32_t sz = 0;
    int rv = read_full(connfd, &rbuf[0], 4);
    if (rv) {
        msg("unable to read msg size", -1);
        return -1;
    }
    memcpy(&sz, &rbuf[0], 4);
    printf("msg size is %d\n", sz);
    if (sz > K_MAX_LENGTH) {
        msg("msg size too large", -1);
        return -1;
    }

    rv = read_full(connfd, &rbuf[4], (size_t) sz);
    uint32_t rescode = 0;
    memcpy(&rescode, &rbuf[4], 4);
    printf("server says: [%u] %.*s\n", rescode, sz - 4, &rbuf[4 + 4]);

    return 0;
}

int main(int argc, char** argv) {
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

    std::vector<std::string> cmd;
    for (int i = 1; i < argc; i++) {
        cmd.push_back(argv[i]);
    }
    int32_t err = send_req(fd, cmd);
    if (err) {
        goto L_DONE;
    }

    err = read_res(fd);
    if (err) {
        goto L_DONE;
    }

    // mimik a client hanging and ready to make another request
    while (true) {}
    
L_DONE:
    close(fd);
    return 0;
}
