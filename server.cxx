#include <cstring>
#include <string>
#include <sys/_types/_fd_def.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <vector>
#include <poll.h>
#include "file_ops.h"
#include "util.h"
#include "ops.h"

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2, // mark connections for deletion
};

typedef struct Conn {
    int fd = -1;
    uint32_t state = 0; // either STATE_REQ or STATE_RES
    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + K_MAX_LENGTH];
    size_t rbuf_offset = 0;
    // buffer for writing;
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + K_MAX_LENGTH]; 
} Conn;

bool try_flush_buffer(Conn* conn) {
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // stop
        return false;
    }
    if (rv < 0) {
        msg("unexpected error", errno);
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t) rv;
    assert (conn->wbuf_sent <= conn->wbuf_size);
    if (conn->wbuf_sent == conn->wbuf_size) {
        conn->wbuf_size = 0;
        conn->wbuf_sent = 0;
        conn->state = STATE_REQ;
        return false;
    }
    return true;
}

void state_res(Conn* conn) {
    while (try_flush_buffer(conn)) {}
}

int32_t parse_req(
    const uint8_t* req_buf, int32_t len,
    std::vector<std::string>& cmd
) {
    if (len < 4) {
        msg("not enough data in req_buf", 0);
        return -1;
    }
    // number of strings
    uint32_t n = 0;
    memcpy(&n, req_buf, 4);
    if (n > K_MAX_LENGTH) {
        msg("too many strs", 0);
        return -1;
    }

    size_t pos = 4;
    while (n--) {
        if (pos + 4 > len) {
            msg("not enough data: |nstr|len (partial)|", 0);
            return -1;
        }
        uint32_t str_len = 0;
        memcpy(&str_len, &req_buf[pos], 4); 
        if (pos + str_len + 4 > len) {
            msg("not enough data: |nstr|len|data (partial)", 0);
            return -1;
        }
        cmd.push_back(std::string((char *)&req_buf[pos + 4], str_len));
        pos += 4 + str_len;
    }
    if (pos != len) {
        msg("trailing garbage", 0);
        return -1;
    }
    return 0;
}

int32_t do_request(const uint8_t *req_buf, 
        int32_t len, uint32_t *rescode, 
        uint8_t *res_buf, uint32_t *res_len) {
    std::vector<std::string> cmd;
    if (0 != parse_req(req_buf, len, cmd)) {
        msg("bad request", -1);
        return -1;
    }
    if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        printf("get request\n");
        *rescode = do_get(cmd, res_buf, res_len);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        printf("set request\n");
        *rescode = do_set(cmd, res_buf, res_len);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        printf("del request\n");
        *rescode = do_del(cmd, res_buf, res_len);
    } else {
        *rescode = RES_ERR;
        const char *msg = "Unknown cmd";
        strcpy((char *)res_buf, msg);
        *res_len = strlen(msg);
        return 0;
    }
    return 0;
}

bool try_one_request(Conn* conn) {
    if (conn->rbuf_size < 4) {
        msg("not getting enough data, try in the next iteration.", 0);
        return false;
    }

    int len = 0;
    memcpy(&len, &conn->rbuf[conn->rbuf_offset], 4);
    if (len > K_MAX_LENGTH) {
        msg("message length is too long", 0);
        return false;
    }
    
    if (4 + len > conn->rbuf_size) {
        msg("not enough data available, try in the next iteration", 0);
        return false;
    }
    // got one request, process it.
    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = do_request(
        &conn->rbuf[4 + conn->rbuf_offset], len, 
        &rescode, &conn->wbuf[4 + 4 + conn->wbuf_size], &wlen
    );
    if (err) {
        conn->state = STATE_END;
        return false;
    }

    // generating response.
    wlen += 4;
    printf("response length %d\n", wlen);
    // package |wlen|rescode|data| into wbuf
    memcpy(&conn->wbuf[conn->wbuf_size], &wlen, 4);
    memcpy(&conn->wbuf[conn->wbuf_size + 4], &rescode, 4);
    conn->wbuf_size += 4 + wlen;

    // remove the request from rbuf.
    // note: frequent memmove is inefficient, need better handling in prod.
    size_t remain = conn->rbuf_size - len - 4;
    conn->rbuf_offset += len + 4;
    conn->rbuf_size = remain;

    // continue the outer loop if the request was fully processed
    return (conn->state == STATE_REQ);
}

void conn_put(std::vector<Conn * > &fd2conn, Conn* new_conn) {
    if ((size_t) new_conn->fd >= fd2conn.size()) {
        fd2conn.resize(new_conn->fd + 1);
        printf("resized fd2conn\n");
    }
    fd2conn[new_conn->fd] = new_conn;
}

int accept_new_connection(std::vector<Conn *> &fd2conn, int fd) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        return -1; // error
    }
    printf("accepted new connection\n");

    fd_set_nb(connfd);
    Conn *new_conn = (Conn *) malloc(sizeof(Conn));
    printf("new_conn addr: %p\n", new_conn);
    if (!new_conn) {
        close(connfd);
        return -1;
    }
    new_conn->fd = connfd;
    new_conn->state = STATE_REQ;
    new_conn->rbuf_size = 0;
    new_conn->wbuf_size = 0;
    new_conn->wbuf_sent = 0;
    conn_put(fd2conn, new_conn);
    return 0;
}


bool try_fill_buffer(Conn* conn) {
    assert (conn->rbuf_size < sizeof(conn->rbuf));
    
    ssize_t rv = 0;
    if (conn->rbuf_offset) {
        assert (conn->rbuf_offset <= sizeof(conn->rbuf));
        memmove(conn->rbuf, &conn->rbuf[conn->rbuf_offset], conn->rbuf_size);
        conn->rbuf_offset = 0;
    }
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // stop
        msg("try to read fd, but read fd not ready", errno);
        return false;
    }
    if (rv < 0) {
        // error
        msg("unexpected error", errno);
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            msg("unexpected EOF", errno);
        } else {
            msg("EOF", errno);
        }
        conn->state = STATE_END;
        return false; 
    }
    conn->rbuf_size += (size_t) rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    // Try to process request one at a time.
    while (try_one_request(conn)) {}
    
    // change state
    conn->state = STATE_RES;
    state_res(conn);

    return (conn->state == STATE_REQ);
}

void state_req(Conn* conn) {
    while (try_fill_buffer(conn)) {}
}


void connection_io(Conn* conn) {
    if (conn->state == STATE_REQ) {
        printf("connection starts requests\n");
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        printf("connection starts response\n");
        state_res(conn);
    } else {
        assert(0);
    }
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

    // map of all client connections, keyed by fd.
    std::vector<Conn *> fd2conn;

    // set the listen 
    fd_set_nb(fd);

    std::vector<struct pollfd> poll_args;

    while (true) {
        poll_args.clear();
        // for convenience, the listening fd is put in the first position.
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        // connection fds
        for (Conn *conn : fd2conn) {
            if (!conn) {
                continue;
            }
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events |= POLLERR;
            poll_args.push_back(pfd);
            pfd.revents = 0;
        }
        
        // poll for the active fds.
        // timeout doesn't matter here
        int rv = poll(poll_args.data(), (nfds_t) poll_args.size(), 1000);
        if (rv < 0) {
            die("poll", errno);
        }

        // process active fds
        for (int i = 1; i < poll_args.size(); i++) {
            pollfd poll_arg = poll_args[i];
            if (poll_arg.revents) {
                printf("fd %d has new request\n", poll_arg.fd);
                Conn *conn = fd2conn[poll_arg.fd]; 
                connection_io(conn);
                if (conn->state == STATE_END) {
                    printf("conn fd %d state reach to the end, freeing the memory\n", poll_arg.fd);
                    fd2conn[conn->fd] = NULL;
                    (void) close(conn->fd);
                    free(conn);
                }
            }
        }

        // try to accept new connection
        if (poll_args[0].revents) {
            (void) accept_new_connection(fd2conn, fd);
        }
    }
    return 0;
}
