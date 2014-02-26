/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWRBTREE_H_
#define INCLUDE_YWRBTREE_H_

#include <ywcommon.h>

typedef int32_t key_t;

void        *rb_create_tree();
void         rb_delete_tree(void *rbt);
bool         rb_insert(void *rbt, key_t key, void * data);
bool         rb_remove(void *rbt, key_t key);
void         rb_print(void *rbt);
void        *rb_find(void *rbt, key_t key);
void         rb_validation(void *rbt);
void         rb_infix(void * rbt);

int64_t      rb_get_compare_count();
void         rbtree_concunrrency_test(void * rbt);

#endif  // INCLUDE_YWRBTREE_H_
