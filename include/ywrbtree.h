/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWRBTREE_H_
#define INCLUDE_YWRBTREE_H_

#include <ywcommon.h>

typedef int32_t key_t;
typedef int32_t node_color_t;
static const node_color_t RB_RED     = 0;
static const node_color_t RB_BLACK   = 1;

typedef int32_t node_side_t;
static const node_side_t RB_LEFT     = 0;
static const node_side_t RB_RIGHT    = 1;
static const node_side_t RB_SIDE_MAX = 2;

typedef struct  nodeStruct node_t;
struct nodeStruct {
    node_color_t   color;
    node_t       * parent;
    node_t       * child[RB_SIDE_MAX];
    key_t          key;
};

node_t      *rb_create_tree();
bool         rb_insert(node_t **root, key_t key);
bool         rb_remove(node_t **root, key_t key);
bool         rb_print(int level, node_t *node);
int32_t      rb_validation(node_t *root);
void         rb_infix(node_t *root);

#endif  // INCLUDE_YWRBTREE_H_
