/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWBTREE_H_
#define INCLUDE_YWBTREE_H_

#include <ywcommon.h>

void *bt_create();
bool  bt_insert(void *bt, char *key, char *data);
bool  bt_remove(void *bt, char *key);
char *bt_find(void *bt, char *key);

void btree_basic_test();

#endif  // INCLUDE_YWBTREE_H_

