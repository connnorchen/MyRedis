#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "util.h"

// this function reads until it gets exactly n bytes;
int32_t read_full(int conn, char* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(conn, buf, n); 
        if (rv <= 0) {
            // eof or unexpected error. 
            // If no information is put in, then read will be blocked. 
            return -1; 
        }

        // assert((size_t) rv <= n);
        n -= (size_t) rv;
        buf += (size_t) rv;
    }   
    return 0;
}

// this function writes until it gets exactly n bytes;
int32_t write_all(int conn, char* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(conn, buf, n);
        if (rv <= 0) {
            return -1; // error;
        }
        
        // assert ((size_t) rv <= n);
        n -= (size_t) rv;
        buf += (size_t) rv;
    }
    return 0;
}

// gets the length from connfd and reads it into buf
// buf memory allocation will be allocated by the caller. 
// return length. Error case < 0. 
int32_t read_length_from_socket(int connfd, char buf[]) {
    int32_t err = read_full(connfd, buf, 4);
    if (err) {
        if (errno == 0) {
            printf("EOF\n");
        } else {
            msg("read_full with int failed\n", errno);
            printf("errno: %d\n", errno);
        }
        // error case
        return -1;
    }

    size_t length = 0;
    memcpy(&length, buf, 4);
    return length;
}

// gets the content from connfd and reads it into buf
// memory allocation will be allocated by the caller. 
// return length. Error case < 0. 
int32_t read_content_from_socket(int connfd, char buf[], size_t length) {
    int32_t err = read_full(connfd, buf, length);
    if (err) {
        if (errno == 0) {
            printf("EOF\n");
        } else {
            msg("read_full with int failed\n", errno);
            printf("errno: %d\n", errno);
        }
        // error case
        return -1;
    }
    
    return 0;
}
