#include <stdlib.h>

typedef struct HeapItem {
    uint64_t val = 0;
    size_t *ref;
} HeapItem;
void heap_update(HeapItem *a, size_t pos, size_t len);
