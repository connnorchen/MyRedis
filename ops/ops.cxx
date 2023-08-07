#include <string>
#include <vector>
#include "util.h"
#include "file_ops.h"
#include "ops.h"
#include "serialize.h"


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
    std::vector<std::string> &cmd, std::string &out, HMap *db
) {
    (void)cmd;
    out_arr(out, (uint32_t)hm_size(db));    
    hm_scan(db, scan_key, (void *) &out); 
}

void do_get(
    std::vector<std::string> &cmd, std::string &out, HMap *db) {
    Entry key;
    
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());

    HNode *node = hm_lookup(db, &key.node, &entry_eq);
    if (!node) {
        out_nil(out);
        return;
    }
    
    Entry *res_entry = ((Entry *)container_of(node, Entry, node));
    if (res_entry->type != T_STR) {
        return out_err(out, ERR_TYPE, "You can't 'get' a SET type value");
    }
    std::string &res = res_entry->value;
    assert (res.size() <= K_MAX_LENGTH);
    return out_str(out, res);
}

void do_set(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());
    
    HNode *node = hm_lookup(db, &key.node, &entry_eq);
    if (node) {
        container_of(node, Entry, node)->value.swap(cmd[2]);
    } else {
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->value.swap(cmd[2]);
        ent->type = T_STR;
        ent->node.hcode = key.node.hcode;
        hm_insert(db, &ent->node);
    }
    printf("setting out to be nil");
    return out_nil(out);
}

void do_expire(std::vector<std::string> &cmd, std::string &out, 
        HMap *db, void (*entry_set_ttl)(Entry *, int64_t)) {
    int64_t ttl_ms = 0;
    if (!str2ll(cmd[2], ttl_ms)) {
        return out_err(out, ERR_ARG, "expect int64");
    }

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(db, &key.node, &entry_eq);
    if (node) {
        Entry *ent = container_of(node, Entry, node);
        entry_set_ttl(ent, ttl_ms);
    }
    return out_int(out, node ? 1: 0);
}

void do_ttl(std::vector<std::string> &cmd, std::string &out, 
        HMap *db, std::vector<HeapItem> &heap) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(db, &key.node, &entry_eq);
    if (!node) {
        return out_int(out, -2);
    }

    Entry *ent = container_of(node, Entry, node);
    if (ent->heap_idx == (size_t)-1) {
        return out_int(out, -1);
    }

    uint64_t expire_at = heap[ent->heap_idx].val;
    uint64_t now_us = get_mononic_usec();
    return out_int(out, expire_at > now_us ? (expire_at - now_us) / 1000 : 0);
}

void do_del(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());
    
    HNode *node = hm_pop(db, &key.node, &entry_eq);
    if (node) {
        delete container_of(node, Entry, node);
    }
    return out_int(out, node ? 1 : 0);
}

// zadd zset_name score name
void do_zadd(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    double score = 0;
    if (!str2dbl(cmd[2], score)) {
        return out_err(out, ERR_ARG, "you must pass in score to zset");
    }

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());

    HNode *node = hm_lookup(db, &key.node, &entry_eq);
    Entry *ent = NULL;
    if (!node) {
        ent = new Entry();
        ent->key.swap(key.key);
        ent->type = T_ZSET;
        ent->node.hcode = key.node.hcode;
        ent->zset = new ZSet();
        // add to global key space db
        hm_insert(db, &ent->node);
    } else {
        ent = container_of(node, Entry, node);
        if (ent->type != T_ZSET) {
            return out_err(out, ERR_TYPE, "You can't zadd an existing non-zset key");
        }
    }
    const std::string &name = cmd[3];
    bool res = zset_add(ent->zset, name.data(), name.size(), score);
    return out_int(out, res);
}

// Return true if out is verified as zset and set ent as corresponding entry
static bool verify_zset(std::string &out, std::string &name, Entry **ent, HMap *db) {
    Entry key;
    key.key.swap(name);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *hnode = hm_lookup(db, &key.node, &entry_eq);
    if (!hnode) {
        out_nil(out);
        return false;
    }
    *ent = container_of(hnode, Entry, node);
    if ((*ent)->type != T_ZSET) {
        out_err(out, ERR_TYPE, "expected zset");
        return false;
    }
    return true;
}

// zrem zset_name name
void do_zrem(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    Entry* ent = NULL;
    if (!verify_zset(out, cmd[1], &ent, db)) {
        out_nil(out);
        return;
    }
    std::string &s = cmd[2];
    ZNode *res = zset_pop(ent->zset, s.data(), s.size()); 
    if (res) {
        znode_del(res);
    }
    return out_int(out, res ? 1 : 0);
}

// zscore zset_name name
void do_zscore(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    Entry *ent = NULL;
    if (!verify_zset(out, cmd[1], &ent, db)) {
        out_nil(out);
        return;
    }
    std::string &s = cmd[2];
    ZNode *res = zset_lookup(ent->zset, s.data(), s.size());
    return res ? out_dbl(out, res->score) : out_nil(out); 
}

// zquery zset_name score name offset limit
void do_zquery(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    double score = 0;
    if (!str2dbl(cmd[2], score)) {
        return out_err(
            out, 
            ERR_ARG, 
            "you must pass in double type score to zset"
        );
    }
    
    int64_t offset = 0;
    int64_t limit = 0;
    if (!str2ll(cmd[4], offset)) {
        return out_err(
            out, 
            ERR_ARG, 
            "you must pass in int64_t type offset"
        );
    }
    if (!str2ll(cmd[5], limit)) {
        return out_err(
            out, 
            ERR_ARG, 
            "you must pass in int64_t type limit"
        );
    }

    if (limit <= 0) {
        return out_arr(out, 0);
    }

    Entry *ent = NULL;
    if (!verify_zset(out, cmd[1], &ent, db)) {
        if (out[0] == SER_NIL) {
            out.clear();
            out_arr(out, 0);
        }
        return;
    }

    std::string &name = cmd[3];
    ZNode *node = zset_query(
        ent->zset,
        score,
        name.data(),
        name.size(),
        offset
    );
    if (!node) {
        return out_err(
            out, 
            ERR_ARG, 
            "score, name pair does not exist"
        );
    }
    uint64_t n = 0;
    out_arr(out, n);
    while (node && (int64_t) n < limit * 2) {
        out_dbl(out, node->score);
        out_str(out, std::string(node->name, node->len));
        
        node = container_of(
            avl_offset(&node->avl_node, +1),
            ZNode,
            avl_node
        );
        n += 2;
    }
    return out_update_arr(out, n);
}

// zrank zset_name name
void do_zrank(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    Entry *ent = NULL;
    if (!verify_zset(out, cmd[1], &ent, db)) {
        out_nil(out);
        return;
    }
    
    std::string &name = cmd[2];
    int32_t rank = zrank(ent->zset, name.data(), name.size());
    if (rank < 0) {
        out_err(out, ERR_ARG, "can't find rank, make sure name exists in zset");
        return;
    }
    return out_int(out, rank);
}

// zrank zset_name name
void do_zrrank(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    Entry *ent = NULL;
    if (!verify_zset(out, cmd[1], &ent, db)) {
        out_nil(out);
        return;
    }
    
    std::string &name = cmd[2];
    int32_t rank = zrrank(ent->zset, name.data(), name.size());
    if (rank < 0) {
        out_err(out, ERR_ARG, "can't find rank, make sure name exists in zset");
        return;
    }
    return out_int(out, rank);
}

// zrange zset_name left_bound right_bound
void do_zrange(std::vector<std::string> &cmd, std::string &out, HMap *db) {
    double left_bound = 0;
    double right_bound = 0;
    if (!str2dbl(cmd[2], left_bound)) {
        return out_err(out, ERR_TYPE, "left bound not a double");
    }
    printf("left_bound %f\n", left_bound);
    if (!str2dbl(cmd[3], right_bound)) {
        return out_err(out, ERR_TYPE, "right bound not a double");
    }
    printf("right_bound %f\n", right_bound);
    if (left_bound > right_bound) {
        printf("left_bound %f\n", left_bound);
        printf("right_bound %f\n", right_bound);
        return out_int(out, 0);
    }

    Entry *ent = NULL;
    if (!verify_zset(out, cmd[1], &ent, db)) {
        out_nil(out);
        return;
    }
    return out_int(out, zrange(ent->zset, left_bound, right_bound));
}
