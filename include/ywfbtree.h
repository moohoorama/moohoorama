/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
/* FixedsizeBTree(FBTree)*/

#ifndef INCLUDE_YWFBTREE_H_
#define INCLUDE_YWFBTREE_H_

#include <ywcommon.h>

typedef uint32_t     fbKey;

void *fb_create();
bool  fb_insert(void *_fbt, fbKey key, void *data);
bool  fb_remove(void *_fbt, fbKey key);
void *fb_find(void *_fbt, fbKey key);
void  fb_dump(void *_fbt);

void  fb_basic_test();

#endif  // INCLUDE_YWFBTREE_H_


