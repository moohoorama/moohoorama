/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywbtree.h>
#include <ywworker.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>
#include <ywmempool.h>

int32_t ywb_int_comp(char *_left, char *_right) {
    int32_t *left = reinterpret_cast<int32_t*>(_left);
    int32_t *right= reinterpret_cast<int32_t*>(_right);

    return *right - *left;
}

int32_t ywb_get_size(char * /*ptr*/) {
    return 4;
}

void btree_basic_test() {
//    ywBTree<ywb_int_comp, ywb_get_size> btree;
}
