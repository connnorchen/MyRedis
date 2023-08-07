#include <vector>
#include <string>
#include "zset.h"
#include "linked_list.h"
#include "heap.h"

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
    // for TTL
    size_t heap_idx = -1;
} Entry;

void do_keys(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_get(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_set(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_del(std::vector<std::string> &cmd, std::string &out, HMap *db, void (*entry_del)(Entry *));
void do_zadd(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_zrem(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_zscore(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_zquery(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_zrank(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_zrrank(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_zrange(std::vector<std::string> &cmd, std::string &out, HMap *db);
void do_expire(std::vector<std::string> &cmd, std::string &out, HMap *db, void (*entry_set_ttl)(Entry *, int64_t));
void do_ttl(std::vector<std::string> &cmd, std::string &out, HMap *db, std::vector<HeapItem> &heap);
