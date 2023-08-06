#include <stdlib.h>
typedef struct DList {
    DList *prev = NULL;
    DList *next = NULL;
} DList;

inline void dlist_init(DList *node) {
    node->prev = node->next = node;
};

inline bool dlist_empty(DList *node) {
    return node->next == node;
};

inline void dlist_insert_before(DList *target, DList *rookie) {
    DList *prev = target->prev;
    prev->next = rookie;
    rookie->prev = prev;
    rookie->next = target;
    target->prev = rookie;
};

inline void dlist_detach(DList *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
};
