#include <string>

enum {
    ERR_UNKNOWN = 1,
    ERR_2BIG = 2,
};

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})

bool cmd_is(const std::string &, const char *);
void die(const char*, int);
void msg(const char*, int);
