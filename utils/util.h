#include <string>

enum {
    ERR_UNKNOWN = 1,
    ERR_2BIG = 2,
    ERR_ARG = 3,
    ERR_TYPE = 4,
    ERR_DELETED = 5,
};

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})

bool cmd_is(const std::string &, const char *);
void die(const char*, int);
void msg(const char*, int);
size_t min(size_t lhs, size_t rhs);
bool str2dbl(const std::string &s, double &out);
bool str2ll(const std::string &s, int64_t &out);
uint64_t str_hash(const uint8_t *data, size_t len);
uint64_t get_mononic_usec();
