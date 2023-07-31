#include <cstring> 
#include <iostream>
#include <sstream>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <sys/_types/_int32_t.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <vector>
#include "serialize.h"
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

static int32_t on_response(const uint8_t *data, size_t sz) {
    if (sz < 1) {
        msg("bad response", 0);
        return -1;
    }
    switch (data[0]) {
    case SER_NIL: {
        printf("(nil)\n");
        return 1;
    }
    case SER_ERR: {
        // |SER_ERR|ERR_CODE|ERRLEN|ERRMSG|
        if (sz < 1 + 8) {
            msg("bad response", 0);
            return -1;
        }
        int32_t err_code = 0;
        memcpy(&err_code, &data[1], 4);
        uint32_t msg_len = 0;
        memcpy(&msg_len, &data[1 + 4], 4);
        if (msg_len > K_MAX_LENGTH) {
            msg("length too long", 0);
            return -1;
        }
        if (sz < 1 + 8 + msg_len) {
            msg("bad response", 0);
            return -1;
        }
        printf("err [%d]: %.*s\n", err_code, msg_len, &data[1 + 4 + 4]);
        return 1 + 8 + msg_len;
    }
    case SER_STR: {
        // |SER_STR|STR_LEN|STR|
        if (sz < 1 + 4) {
            msg("bad response", 0);
            return -1;
        }
        uint32_t msg_len = 0;
        memcpy(&msg_len, &data[1], 4);
        if (msg_len > K_MAX_LENGTH) {
            msg("length too long", 0);
            return -1;
        }
        if (sz < 1 + 4 + msg_len) {
            msg("bad response", 0);
            return -1;
        }
        printf("str: %.*s\n", msg_len, &data[5]);
        return 1 + 4 + msg_len;
    }
    case SER_INT: {
        // |SER_INT|INT
        if (sz < 1 + 8) {
            msg("bad response", 0);
            return -1;
        }
        int64_t res = 0;
        memcpy(&res, &data[1], 8);
        printf("int: %lld\n", res);
        return 1 + 8;
    }
    case SER_ARR: {
        // |SER_ARR|INT|
        if (sz < 1 + 4) {
            msg("bad response", 0);
            return -1;
        }
        uint32_t len = 0;
        memcpy(&len, &data[1], 4);
        printf("arr: len (%d)\n", len);
        size_t cur_size = 1 + 4;
        for (int i = 0; i < len; i++) {
            int32_t rv = on_response(&data[cur_size], sz - cur_size);
            if (rv < 0) {
                return rv;
            }
            cur_size += rv;
        }
        printf("arr end\n");
        return (int32_t) cur_size;
    }
    default:
        msg("bad response");
        return -1;
    }
}

static int32_t read_res(int connfd) {
    char rbuf[4 + K_MAX_LENGTH + 1];
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
    printf("read %d bytes\n", sz);
    on_response((uint8_t *) &rbuf[4], sz);
    return 0;
}

int32_t process_req(int fd, std::vector<std::string> cmd) {
    int32_t err = send_req(fd, cmd);
    if (err) {
        return err;
    }
    
    
    err = read_res(fd);
    if (err) {
        std::stringstream s;
        for (auto it = cmd.begin(); it != cmd.end(); it++)    {
            if (it != cmd.begin()) {
                s << " ";
            }
            s << *it;
        }
 
        printf("error [%d]: %s\n", err, s.str().c_str());
    }
    return 0;
}

int send_one_request(int argc, char** argv, int fd) {
    std::vector<std::string> cmd;
    for (int i = 1; i < argc; i++) {
        cmd.push_back(argv[i]);
    }
    int32_t err = send_req(fd, cmd);
    if (err) {
        return err;
    }

    err = read_res(fd);
    if (err) {
        return err;
    }
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
    if (send_one_request(argc, argv, fd)) {
        goto L_DONE;
    }
    // for (int i = 0; i < 100000; i++) {
    //     std::vector<std::string> cmd1 = {"set", std::to_string(i), std::to_string(i)};
    //     process_req(fd, cmd1);
    // }

    // for (int i = 0; i < 100000; i++) {
    //     std::vector<std::string> cmd1 = {"get", std::to_string(i)};
    //     process_req(fd, cmd1);
    // }

    // for (int i = 0; i < 100000; i++) {
    //     std::vector<std::string> cmd1 = {"del", std::to_string(i)};
    //     process_req(fd, cmd1);
    // }

    // mimik a client hanging and ready to make another request
    while (true) {}
    
L_DONE:
    close(fd);
    return 0;
}

