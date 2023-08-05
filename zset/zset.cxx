#include "zset.h"
#include "util.h"
#include <cstring>

static ZNode *znode_new(const char *name, 
        double score, size_t len) {
    ZNode *new_znode = (ZNode *) malloc(sizeof(ZNode) + len);
    assert (new_znode);
    avl_init(&new_znode->avl_node);
    new_znode->h_node.next = NULL;
    new_znode->h_node.hcode = str_hash((uint8_t *)name, len);
    printf("initialize hcode %llu\n", new_znode->h_node.hcode);
    new_znode->len = len;
    new_znode->score = score;
    memcpy(&new_znode->name[0], name, len);
    return new_znode;
}

// compare by (score, name) tuple
static bool zless(
    AVLNode *lhs, double score, const char *name, size_t len
) {
    ZNode *zl = container_of(lhs, ZNode, avl_node);
    if (zl->score != score) {
        return zl->score < score;
    }
    int rv = memcmp(zl->name, name, len);
    if (rv != 0) {
        return rv < 0;
    }
    return zl->len < len;
}

static bool zless(
    AVLNode *lhs, AVLNode *rhs
) {
    ZNode *zr = container_of(rhs, ZNode, avl_node);
    return zless(lhs, zr->score, zr->name, zr->len);
}

static bool zless_score(
    AVLNode *node, double score
) {
    ZNode *znode = container_of(node, ZNode, avl_node);
    return znode->score < score;
}

static bool zgreater_score(
    AVLNode *node, double score
) {
    ZNode *znode = container_of(node, ZNode, avl_node);
    return znode->score > score;
}

static bool hcmp(
    HNode *node, HNode *key
) {
    printf("node->hcode %llu\n", node->hcode);
    printf("key->hcode %llu\n", key->hcode);
    if (node->hcode != key->hcode) {
        return false;
    }
    ZNode *znode = container_of(node, ZNode, h_node); 
    HKey *hkey = container_of(key, HKey, node);
    printf("znode->len %lu\n", znode->len);
    printf("hkey->len %lu\n", hkey->len);
    if (znode->len != hkey->len) {
        return false;
    }
    return memcmp(znode->name, hkey->name, znode->len) == 0;
}

static void tree_add(ZSet *zset, ZNode *znode) {
    if (!zset->tree) {
        zset->tree = &znode->avl_node;
        return;
    }
    
    AVLNode *cur = zset->tree;
    while (true) {
        AVLNode **from = zless(&znode->avl_node, cur)
            ? &cur->left : &cur->right;
        if (!*from) {
            *from = &znode->avl_node;
            znode->avl_node.parent = cur;
            zset->tree = avl_fix(&znode->avl_node);
            return;
        }
        cur = *from;
    }
}

// update the score of an existing node (AVL tree reinsertion)
static void zset_update(ZSet *zset, ZNode *znode, double score) {
    if (score == znode->score) {
        return;
    }
    zset->tree = avl_del(&znode->avl_node);
    znode->score = score;
    avl_init(&znode->avl_node);
    tree_add(zset, znode);
}

ZNode *zset_lookup(ZSet *zset, const char *name, size_t len) {
    if (!zset->tree) {
        return NULL;
    }

    HKey key;
    key.node.hcode = str_hash((uint8_t *)name, len);
    printf("key hcode %llu\n", key.node.hcode);
    key.name = name;
    key.len = len;

    HNode *h_node = hm_lookup(&zset->hmap, &key.node, &hcmp);
    printf("h_node look up: %p\n", h_node);
    if (h_node) {
        return container_of(h_node, ZNode, h_node);
    }
    return NULL;
}

// add a new (score, name) tuple, or update the score of the existing tuple
bool zset_add(ZSet *zset, const char *name, size_t len, double score) {
    ZNode *node = zset_lookup(zset, name, len);
    if (node) {
        zset_update(zset, node, score);
        return false;
    } else {
        node = znode_new(name, score, len);
        tree_add(zset, node);
        hm_insert(&zset->hmap, &node->h_node);
        return true;
    }
}

// deletion by name
ZNode *zset_pop(ZSet *zset, const char *name, size_t len) {
    if (!zset->tree) {
        return NULL; 
    }

    HKey key;
    key.name = name;
    key.len = len;
    key.node.hcode = str_hash((uint8_t *)name, len);

    HNode *node = hm_pop(&zset->hmap, &key.node, hcmp);
    if (!node) {
        return NULL;
    }

    ZNode *znode = container_of(node, ZNode, h_node);
    zset->tree = avl_del(&znode->avl_node);
    return znode;
}

static ZNode *zset_fge_score(ZSet *zset, double score) {
    if (!zset->tree) {
        return NULL;
    }

    AVLNode *node = zset->tree;
    AVLNode *found = NULL;
    while (node) {
        if (zless_score(node, score)) {
            node = node->right;
        } else {
            found = node;
            node = node->left;
        }
    }
    return container_of(found, ZNode, avl_node);
}

static ZNode *zset_fle_score(ZSet *zset, double score) {
    if (!zset->tree) {
        return NULL;
    }

    AVLNode *node = zset->tree;
    AVLNode *found = NULL;
    while (node) {
        if (zgreater_score(node, score)) {
            node = node->left;
        } else {
            found = node;
            node = node->right;
        }
    }
    return container_of(found, ZNode, avl_node);
}

// find the (score, name) tuple that is greater or equal to the argument,
// then offset relative to it.
ZNode *zset_query(
    ZSet *zset, double score, const char *name, 
    size_t len, int64_t offset) {
    if (!zset->tree) {
        return NULL;
    }

    AVLNode *node = zset->tree;
    AVLNode *found = NULL;
    while (node) {
        if (zless(node, score, name, len)) {
            node = node->right;
        } else {
            found = node;
            node = node->left;
        }
    }
    if (found) {
        found = avl_offset(found, offset);
    }
    return found ? container_of(found, ZNode, avl_node) : NULL;
}

// find the zrank of a given znode. (increasing order)
int32_t zrank(ZSet *zset, const char *name, size_t len) {
    if (!zset->tree) {
        return -1;
    }
    ZNode *found = zset_lookup(zset, name, len);
    if (!found) {
        return -1;
    }
    return avl_rank(&found->avl_node);
}

// find the reverse rank of a given znode. (decreasing order)
int32_t zrrank(ZSet *zset, const char* name, size_t len) {
    if (!zset->tree) {
        return -1;
    }
    int32_t rank = zrank(zset, name, len);
    if (rank == -1) {
        return -1;
    }
    return avl_cnt(zset->tree) - rank + 1;
}

int32_t zrange(ZSet *zset, double left, double right) {
    ZNode *fge_left = zset_fge_score(zset, left);
    ZNode *fle_right = zset_fle_score(zset, right);

    int32_t right_rank = zrank(zset, fle_right->name, fle_right->len);
    int32_t left_rank = zrank(zset, fge_left->name, fge_left->len);

    printf("right_rank %d\n", right_rank);
    printf("left_rank %d\n", left_rank);
    return right_rank - left_rank + 1;    
}

void znode_del(ZNode *node) {
    free(node);
}

void dispose_tree(AVLNode *node) {
    if (!node) {
        return;
    }
    dispose_tree(node->left);
    dispose_tree(node->right);
    znode_del(container_of(node, ZNode, avl_node));
}

void dispose_zset(ZSet *zset) {
    dispose_tree(zset->tree);
    hm_destroy(&zset->hmap);
}
