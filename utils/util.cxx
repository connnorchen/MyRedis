#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "util.h"

bool cmd_is(const std::string &word, const char *cmd) {
    return 0 == strcasecmp(word.c_str(), cmd);
}

void die(const char *msg, int err) {
    fprintf(stderr, "[%d] %s\n", err, msg); 
    abort();
}

void msg(const char *msg, int err) {
    fprintf(stderr, "err: [%d] %s\n", err, msg);
}
