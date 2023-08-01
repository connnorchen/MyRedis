#include <stdlib.h>

typedef struct AVLNode {
    uint32_t depth = 0;
    uint32_t cnt = 0;
    AVLNode *left = NULL;
    AVLNode *right = NULL;
    AVLNode *parent = NULL;
} AVLNode;

AVLNode *avl_fix(AVLNode *node);
void avl_init(AVLNode *node);
AVLNode *avl_del(AVLNode *node);
uint32_t avl_depth(AVLNode *node);
uint32_t avl_cnt(AVLNode *node);
uint32_t max(uint32_t lhs, uint32_t rhs);
void avl_update(AVLNode *node);
