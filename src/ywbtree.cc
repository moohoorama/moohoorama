/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywbtree.h>
#include <ywworker.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>
#include <ywmempool.h>
#include <ywtypes.h>

void btree_basic_test() {
    static const int32_t                     count = 1000000;
    typedef ywBTree<ywInt, ywPtr,  1*KB>     btree_type;
    btree_type                               btree;
    ywInt                                    *val = new ywInt[count];
    int32_t                                  i;

    for (i = 0; i < count; ++i) {
//        val[i] = ywInt(count-i);
        val[i] = ywInt(i);
        btree.insert(val[i], &val[i]);
    }
//    btree.dump();

    for (i = 0; i < count; ++i) {
//        val[i] = ywInt(count-i);
        val[i] = ywInt(i);
        assert(btree.find(val[i]) == &val[i]);
    }
    delete val;
}
