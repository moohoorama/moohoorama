/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywbtree.h>
#include <ywworker.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>
#include <ywmempool.h>
#include <ywtypes.h>

void btree_basic_test() {
    static const int32_t  count = 6;
    ywBTree<ywInt>        btree;
    ywInt                 val[count];
    int32_t               i;

    for (i = 0; i < count; ++i) {
        val[i] = ywInt(count-i);
        btree.insert(val[i], &val[i]);
    }
    btree.dump();

    for (i = 0; i < count; ++i) {
        val[i] = ywInt(count-i);
        assert(btree.find(val[i]) == &val[i]);
    }
    btree.dump();
}
