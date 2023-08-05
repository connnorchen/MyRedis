#include "avl_trees.h"
#include "hashtable.h"

typedef struct ZSet {
    AVLNode *tree = NULL;
    HMap hmap;
} ZSet;

typedef struct ZNode {
    AVLNode avl_node;
    HNode h_node;
    double score = 0;
    size_t len = 0;
    char name[0];
} ZNode;

// a helper structure for the hashtable lookup
typedef struct HKey {
    HNode node;
    const char *name = NULL;
    size_t len = 0;
} HKey;

bool zset_add(ZSet *zset, const char *name, size_t len, double score);
ZNode *zset_pop(ZSet *zset, const char *name, size_t len);
void znode_del(ZNode *node);
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len);
ZNode *zset_query(ZSet *zset, double score, const char *name, size_t len, int64_t offset);
int32_t zrank(ZSet *zset, const char *name, size_t len);
int32_t zrrank(ZSet *zset, const char* name, size_t len);
int32_t zrange(ZSet *zset, double left, double right);
