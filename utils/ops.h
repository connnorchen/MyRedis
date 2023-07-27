#include <vector>
#include <string>

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

int32_t do_get(const std::vector<std::string>&, uint8_t *, uint32_t *);
int32_t do_set(const std::vector<std::string>&, uint8_t *, uint32_t *);
int32_t do_del(const std::vector<std::string>&, uint8_t *, uint32_t *);
