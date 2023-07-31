#include <string>

enum {
    ERR_UNKNOWN = 1,
    ERR_2BIG = 2,
};

bool cmd_is(const std::string &, const char *);
void die(const char*, int);
void msg(const char*, int);
