#include "avl_trees.h"
#include "util.h"

void avl_init(AVLNode *node) {
    node->depth = 1;
    node->cnt = 1;
    node->left = node->right = node->parent = NULL;
}

uint32_t avl_depth(AVLNode *node) {
    return node ? node->depth : 0;
}

uint32_t avl_cnt(AVLNode *node) {
    return node ? node->cnt : 0;
}

uint32_t max(uint32_t lhs, uint32_t rhs) {
    return lhs < rhs ? rhs : lhs;
}

void avl_update(AVLNode *node) {
    node->depth = 1 + max(avl_depth(node->left), avl_depth(node->right));
    node->cnt = 1 + avl_cnt(node->left) + avl_cnt(node->right);
}

static AVLNode *rot_left(AVLNode *node) {
    AVLNode *new_node = node->right;
    if (new_node->left) {
        new_node->left->parent = node;
    }
    node->right = new_node->left;
    new_node->left = node;
    new_node->parent = node->parent;
    node->parent = new_node;
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

static AVLNode *rot_right(AVLNode *node) {
    AVLNode *new_node = node->left;
    if (new_node->right) {
        new_node->right->parent = node;
    }
    node->left = new_node->right;
    new_node->right = node;
    new_node->parent = node->parent;
    node->parent = new_node;
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

static AVLNode *avl_fix_right(AVLNode *node) {
    if (avl_depth(node->right->right) < avl_depth(node->right->left)) {
        node->right = rot_right(node->right);
    }
    return rot_left(node);
}

static AVLNode *avl_fix_left(AVLNode *node) {
    if (avl_depth(node->left->left) < avl_depth(node->left->right)) {
        node->left = rot_left(node->left);
    }
    return rot_right(node);
}

AVLNode *avl_fix(AVLNode *node) {
    while (true) {
        avl_update(node);
        uint32_t l = avl_depth(node->left);
        uint32_t r = avl_depth(node->right);
        AVLNode **from = NULL;
        if (node->parent) {
            from = (node->parent->left == node)
                ? &node->parent->left : &node->parent->right;
        }
        if (l == r + 2) {
            node = avl_fix_left(node);
        }
        if (r == l + 2) {
            node = avl_fix_right(node);
        }
        if (!from) {
            return node;
        }
        *from = node;
        node = node->parent;
    }
}

AVLNode *avl_del(AVLNode *node) {
    if (node->right == NULL) {
        // no right subtree, removing the current node and attach it with 
        // left subtree
        AVLNode *parent = node->parent;
        if (node->left) {
            node->left->parent = parent;
        }
        if (parent) {
            (parent->left == node ? parent->left : parent->right) = node->left;
            return avl_fix(parent);
        } else {
            return node->left; 
        }
    } else {
        AVLNode *victim = node->right;
        while (victim->left) {
            victim = victim->left;
        }
        AVLNode *root = avl_del(victim);

        *victim = *node;
        if (victim->left) {
            victim->left->parent = victim;
        }
        if (victim->right) {
            victim->right->parent = victim;
        }
        AVLNode *parent = node->parent;
        if (parent) {
            (parent->left == node ? parent->left : parent->right) = victim;
            return root;
        } else {
            // removing root?
            return victim;
        }
    }
}
