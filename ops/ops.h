#include <vector>
#include <string>
#include "zset.h"

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

enum {
    T_STR = 0,
    T_ZSET = 1,
};

// the structure for the key
typedef struct Entry {
    HNode node;
    std::string key;
    std::string value;
    uint32_t type;
    ZSet* zset = NULL;
} Entry;

void do_keys(std::vector<std::string> &cmd, std::string &out);
void do_get(std::vector<std::string> &cmd, std::string &out);
void do_set(std::vector<std::string> &cmd, std::string &out);
void do_del(std::vector<std::string> &cmd, std::string &out);
void do_zadd(std::vector<std::string> &cmd, std::string &out);
void do_zrem(std::vector<std::string> &cmd, std::string &out);
void do_zscore(std::vector<std::string> &cmd, std::string &out);
void do_zquery(std::vector<std::string> &cmd, std::string &out);
void do_zrank(std::vector<std::string> &cmd, std::string &out);
void do_zrrank(std::vector<std::string> &cmd, std::string &out);
void do_zrange(std::vector<std::string> &cmd, std::string &out);
