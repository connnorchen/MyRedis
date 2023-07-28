#include <string>
#include <vector>
#include "file_ops.h"
#include "ops.h"
#include "hashtable.h"

static struct {
    HMap db;
} g_data;

static uint64_t str_hash(const uint8_t *data, size_t len) {
    uint64_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

static bool entry_eq(HNode *lhs, HNode *rhs) {
    Entry *le = container_of(lhs, Entry, node);
    Entry *re = container_of(rhs, Entry, node);
    return lhs->hcode == rhs->hcode && le->key == re->key;
}

int32_t do_get(
    std::vector<std::string> &cmd, uint8_t *res_buf,
    uint32_t *res_len) {
    Entry key;
    
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (!node) {
        return RES_NX;
    }
    
    const std::string &res = ((Entry *)container_of(node, Entry, node))->value;
    assert (res.size() <= K_MAX_LENGTH);
    memcpy(res_buf, res.data(), res.size());
    *res_len = (uint32_t) res.size();
    return RES_OK;
}

int32_t do_set(
    std::vector<std::string> &cmd, uint8_t* res_buf,
    uint32_t *res_len) {
    (void) res_buf;
    (void) res_len;

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());
    
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (node) {
        container_of(node, Entry, node)->value.swap(cmd[2]);
    } else {
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->value.swap(cmd[2]);
        ent->node.hcode = key.node.hcode;
        hm_insert(&g_data.db, &ent->node);
    }
    return RES_OK;
}

int32_t do_del(
    std::vector<std::string> &cmd, uint8_t* res_buf,
    uint32_t *res_len) {
    (void) res_buf;
    (void) res_len;

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());
    
    HNode *node = hm_pop(&g_data.db, &key.node, &entry_eq);
    if (node) {
        delete container_of(node, Entry, node);
    }
    return RES_OK;
}
