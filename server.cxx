#include <cstring>
#include <ctime>
#include <string>
#include <sys/_types/_fd_def.h>
#include <sys/_types/_timespec.h>
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
#include "serialize.h"
#include "linked_list.h"

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2, // mark connections for deletion
};

typedef struct Conn {
    int fd = -1;
    // timer
    uint64_t idle_start = 0;
    DList idle_list;
    // either STATE_REQ or STATE_RES
    uint32_t state = 0;
    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + K_MAX_LENGTH];
    size_t rbuf_offset = 0;
    // buffer for writing;
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + K_MAX_LENGTH]; 
} Conn;

static struct {
    // map of all client connections, keyed by fd.
    std::vector<Conn *> fd2conn;
    
    // timers for idle connections.
    DList idle_list;
} g_data;

const uint64_t k_idle_timeout_ms = 5 * 1000;

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
    printf("sended %lu bytes\n", conn->wbuf_sent);
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

int32_t do_request(std::vector<std::string> &cmd, std::string &out) {
    if (cmd.size() == 1 && cmd_is(cmd[0], "keys")) {
        printf("keys requests\n");
        do_keys(cmd, out);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        printf("get request\n");
        do_get(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        printf("set request\n");
        do_set(cmd, out);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        printf("del request\n");
        do_del(cmd, out);
    } else if (cmd.size() == 4 && cmd_is(cmd[0], "zadd")) {
        printf("zadd request\n");
        do_zadd(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "zrem")) {
        printf("zrem request\n");
        do_zrem(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "zscore")) {
        printf("zscore request\n");
        do_zscore(cmd, out);
    } else if (cmd.size() == 6 && cmd_is(cmd[0], "zquery")) {
        printf("zquery request\n");
        do_zquery(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "zrank")) {
        printf("zrank request\n");
        do_zrank(cmd, out);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "zrrank")) {
        printf("zrrank request\n");
        do_zrrank(cmd, out);
    } else if (cmd.size() == 4 && cmd_is(cmd[0], "zrange")) {
        printf("zrange request\n");
        do_zrange(cmd, out);
    } else {
        out_err(out, ERR_UNKNOWN, "Unknown cmd");
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
    std::vector<std::string> cmd;
    if (parse_req(&conn->rbuf[4], len, cmd)) {
        conn->state = STATE_END;
        return false;
    }
    std::string out;
    do_request(cmd, out);

    // got one request, process it.
    if (4 + out.size() > K_MAX_LENGTH) {
        out.clear();
        out_err(out, ERR_2BIG, "response is too big");
    }
    printf("this is out's size %lu \n", out.size());
    
    // generating response.
    uint32_t wlen = (uint32_t) out.size();
    printf("response length %d\n", wlen);
    // package |wlen|rescode|data| into wbuf
    memcpy(&conn->wbuf[conn->wbuf_size], &wlen, 4);
    memcpy(&conn->wbuf[conn->wbuf_size + 4], out.data(), wlen);
    conn->wbuf_size += 4 + wlen;
    printf("wbuf_size %lu \n", conn->wbuf_size);

    // remove the request from rbuf.
    // note: frequent memmove is inefficient, need better handling in prod.
    size_t remain = conn->rbuf_size - len - 4;
    conn->rbuf_offset += len + 4;
    conn->rbuf_size = remain;

    // continue the outer loop if the request was fully processed
    return (conn->state == STATE_REQ);
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


static uint64_t get_mononic_usec() {
    timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return uint64_t(tv.tv_sec) * 1000000 + tv.tv_nsec / 1000;
}

static uint32_t next_timer_ms() {
    if (dlist_empty(&g_data.idle_list)) {
        return 10000; 
    }
    
    uint64_t now_us = get_mononic_usec();
    DList *next = g_data.idle_list.next;
    Conn *next_conn = container_of(next, Conn, idle_list);
    uint64_t next_start = next_conn->idle_start + k_idle_timeout_ms * 1000; 
    if (next_start <= now_us) {
        // missed
        return 0;
    }
    return (uint32_t) ((next_start - now_us) / 1000);
}

void connection_io(Conn* conn) {
    // wake up by poll, update the idle timer
    // by moving conn in front of the idle list.
    conn->idle_start = get_mononic_usec();
    dlist_detach(&conn->idle_list);
    dlist_insert_before(&g_data.idle_list, &conn->idle_list);

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


void conn_put(Conn* new_conn) {
    if ((size_t) new_conn->fd >= g_data.fd2conn.size()) {
        g_data.fd2conn.resize(new_conn->fd + 1);
        printf("resized fd2conn\n");
    }
    g_data.fd2conn[new_conn->fd] = new_conn;
}

void conn_done(Conn* conn) {
    printf("conn fd %d state reach to the end, freeing the memory\n", conn->fd);
    g_data.fd2conn[conn->fd] = NULL;
    (void)close(conn->fd);
    dlist_detach(&conn->idle_list);
    free(conn);
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
    new_conn->idle_start = get_mononic_usec();
    dlist_insert_before(&g_data.idle_list, &new_conn->idle_list);
    conn_put(new_conn);
    return 0;
}

static void process_timers() {
    uint64_t now_us = get_mononic_usec();
    while (!dlist_empty(&g_data.idle_list)) {
        Conn *next_conn = container_of(g_data.idle_list.next, Conn, idle_list);
        uint64_t next_start = next_conn->idle_start + k_idle_timeout_ms * 1000; 
        if (next_start >= now_us + 1000) {
            break;
        }
        
        printf("removing idle connection %d\n", next_conn->fd);
        conn_done(next_conn);
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

    // set the listener as non-blocking.
    fd_set_nb(fd);

    // set the idle_list
    dlist_init(&g_data.idle_list);

    // the event loop
    std::vector<struct pollfd> poll_args;
    while (true) {
        poll_args.clear();
        // for convenience, the listening fd is put in the first position.
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        // connection fds
        for (Conn *conn : g_data.fd2conn) {
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
        int timeout_ms = (int)next_timer_ms();
        int rv = poll(poll_args.data(), (nfds_t) poll_args.size(), timeout_ms);
        if (rv < 0) {
            die("poll", errno);
        }

        // process active fds
        for (int i = 1; i < poll_args.size(); i++) {
            pollfd poll_arg = poll_args[i];
            if (poll_arg.revents) {
                printf("fd %d has new request\n", poll_arg.fd);
                Conn *conn = g_data.fd2conn[poll_arg.fd]; 
                connection_io(conn);
                if (conn->state == STATE_END) {
                    // client closed normally, or something bad happened.
                    // destroy this connection
                    conn_done(conn);
                }
            }
        }

        // handle timers
        process_timers();

        // try to accept new connection
        if (poll_args[0].revents) {
            (void) accept_new_connection(g_data.fd2conn, fd);
        }
    }
    return 0;
}
