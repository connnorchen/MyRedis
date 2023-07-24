#include <cstring>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "file_ops.h"
#include "util.h"


static int32_t one_request(int connfd) {
    char req_buf[sizeof(int) + K_MAX_LENGTH + 1]; 
    int len = read_length_from_socket(connfd, req_buf);
    if (len < 0) {
        msg("read length error", errno);
        return -1;
    }
    if (len > K_MAX_LENGTH) {
        msg("request length is too large", errno);
        return -1;
    }
    
    int32_t rv = read_content_from_socket(connfd, &req_buf[4], len);
    if (rv) {
        msg("read content error", errno);
        return -1;
    }
    req_buf[sizeof(int) + len] = '\0';
    printf("client says: %s\n", &req_buf[4]);
    
    char response_buf[4 + K_MAX_LENGTH] = {};
    memcpy(response_buf, &len, 4); // copy over length
    memcpy(&response_buf[4], &req_buf[4], len); //copy over request msg.
    int err = write_all(connfd, response_buf, len + 4);
    if (err) {
        msg("unable to write all from reply buf", errno);
        return -1;
    }
    return 0;
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // this is needed for most server applications
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()", errno);
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()", errno);
    }

    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // error
        }

        while (true) {
            // here the server only serves one client connection at once
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }
        close(connfd);
    }
    return 0;
}
