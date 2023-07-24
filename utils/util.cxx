#include <stdio.h>
#include <stdlib.h>

void die(const char *msg, int err) {
    fprintf(stderr, "[%d] %s\n", err, msg); 
    abort();
}

void msg(const char *msg, int err) {
    fprintf(stderr, "err: [%d] %s\n", err, msg);
}
