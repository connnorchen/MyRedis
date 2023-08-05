#include <stdlib.h>
#include <string>

#define K_RESIZING_WORK 128
#define K_MAX_LOAD_FACTOR 8

// hash table node, should be embedded in hashtable
typedef struct HNode {
    HNode *next = NULL;
    uint64_t hcode = 0;
} HNode;

// hash table, simple and fixed-size
typedef struct HTab {
    HNode **tab = NULL;
    size_t mask = 0;
    size_t size = 0;
} HTab;

// the real interface of Hmap, progressive resizing requires two HTab. 
typedef struct HMap {
    HTab ht1;
    HTab ht2;
    size_t resizing_pos = 0;
} HMap;


HNode *hm_lookup(HMap *, HNode *, bool (*)(HNode *, HNode *));
HNode *hm_pop(HMap *, HNode *, bool (*)(HNode *, HNode *));
void hm_insert(HMap *, HNode *);
size_t hm_size(HMap *);
void hm_scan(HMap *, void (*) (HNode *, void *), void *);
void hm_destroy(HMap *hmap);
