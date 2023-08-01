#include <_types/_uint32_t.h>
#include <string>
#include <vector>
#include "util.h"
#include "file_ops.h"
#include "ops.h"
#include "hashtable.h"
#include "serialize.h"

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

static void scan_key(HNode *node, void* arg) {
    std::string &out = *(std::string *)arg;
    out_str(out, container_of(node, Entry, node)->key);
}

void do_keys(
    std::vector<std::string> &cmd, std::string &out
) {
    (void)cmd;
    out_arr(out, (uint32_t)hm_size(&g_data.db));    
    hm_scan(&g_data.db, scan_key, (void *) &out); 
}

void do_get(
    std::vector<std::string> &cmd, std::string &out) {
    Entry key;
    
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (!node) {
        out_nil(out);
        return;
    }
    
    const std::string &res = ((Entry *)container_of(node, Entry, node))->value;
    assert (res.size() <= K_MAX_LENGTH);
    return out_str(out, res);
}

void do_set(std::vector<std::string> &cmd, std::string &out) {
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
    printf("setting out to be nil");
    return out_nil(out);
}

void do_del(std::vector<std::string> &cmd, std::string &out) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());
    
    HNode *node = hm_pop(&g_data.db, &key.node, &entry_eq);
    if (node) {
        delete container_of(node, Entry, node);
    }
    return out_int(out, node ? 1 : 0);
}
