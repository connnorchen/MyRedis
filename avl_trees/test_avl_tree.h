#include "avl_trees.h"

typedef struct Data {
    AVLNode node;
    uint32_t val;
} Data;

typedef struct Container {
    AVLNode *root = NULL;
} Container;
