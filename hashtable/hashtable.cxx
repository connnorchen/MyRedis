#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "hashtable.h"

void msg(const char* message) {
    fprintf(stderr, "%s\n", message);
}

static void h_init(HTab *htab, size_t n) {
    // make sure n is a power of 2
    assert (n > 0 && ((n & (n - 1)) == 0));
    htab->tab = (HNode **) calloc(n, sizeof(HNode *));
    htab->mask = n - 1; 
    htab->size = 0;
} 

static void h_insert(HTab *htab, HNode *node) {
    size_t pos = node->hcode & htab->mask;
    HNode* cur_node = htab->tab[pos];
    node->next = cur_node;
    htab->tab[pos] = node;
    htab->size++;
}

static HNode **h_lookup(HTab *htab, HNode *key, bool (*cmp)(HNode *, HNode *)) {
    if (!htab->tab) {
        return NULL;
    }
    size_t pos = htab->mask & key->hcode;
    HNode **from = &htab->tab[pos];
    
    while (*from) {
        if (cmp(*from, key)) {
            return from;
        }

        from = &((*from)->next);
    }
    return NULL;
}

static HNode *h_detach(HTab *htab, HNode **from) {
    HNode *node = *from;
    *from = (*from)->next;
    htab->size -= 1;
    return node;
}

// a subroutine that helps to transfer nodes in H2 to H1
static void hm_help_resizing(HMap *hmap) {
    if (!hmap || !hmap->ht2.tab) {
        return;
    }
    
    size_t pos = 0;
    while (pos < K_RESIZING_WORK && hmap->ht2.size > 0) {
        HNode **from = &hmap->ht2.tab[hmap->resizing_pos];
        if (!*from) {
            hmap->resizing_pos++;
            continue;
        }

        h_insert(&hmap->ht1, h_detach(&hmap->ht2, from));
        pos++;
    }

    if (hmap->ht2.size == 0) {
        free(hmap->ht2.tab);
        hmap->ht2 = HTab{};
    }
}

static void hm_start_resizing(HMap *hmap) {
    assert (hmap->ht2.tab == NULL);
    hmap->ht2 = hmap->ht1;

    h_init(&hmap->ht1, hmap->ht2.size * 2);
    hmap->resizing_pos = 0;
}

void hm_insert(HMap *hmap, HNode *node) {
    if (!hmap->ht1.tab) {
        h_init(&hmap->ht1, 4);
    }
    h_insert(&hmap->ht1, node);

    if (!hmap->ht2.tab) {
        size_t load_factor = hmap->ht1.size / (hmap->ht1.mask + 1);
        if (load_factor >= K_MAX_LOAD_FACTOR) {
            hm_start_resizing(hmap);
        }
    }
    hm_help_resizing(hmap);
}

HNode *hm_lookup(
    HMap *hmap, HNode *key, bool (*cmp)(HNode *, HNode *)
) {
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap->ht1, key, cmp);
    if (!from) {
        from = h_lookup(&hmap->ht2, key, cmp);
    }
    
    return from ? *from : NULL;
}

HNode *hm_pop(
    HMap *hmap, HNode *key, bool (*cmp)(HNode *, HNode *)
) {
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap->ht1, key, cmp);
    if (from) {
        return h_detach(&hmap->ht1, from);
    }

    from = h_lookup(&hmap->ht2, key, cmp);
    if (from) {
        return h_detach(&hmap->ht2, from);
    }
    return NULL;
}

void hm_destroy(HMap *hmap) {
    assert(hmap->ht1.size + hmap->ht2.size == 0);
    free(hmap->ht1.tab);
    free(hmap->ht2.tab);
    *hmap = HMap{};
}
