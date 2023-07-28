#include <vector>
#include <string>

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};
#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})

int32_t do_get(std::vector<std::string>&, uint8_t *, uint32_t *);
int32_t do_set(std::vector<std::string>&, uint8_t *, uint32_t *);
int32_t do_del(std::vector<std::string>&, uint8_t *, uint32_t *);
