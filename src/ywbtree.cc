/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywbtree.h>
#include <ywworker.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>
#include <ywmempool.h>
#include <ywtypes.h>

template<int32_t PAGE_SIZE>
void btree_basic(ywBTree<ywLong, ywPtr, PAGE_SIZE> *btree,
                 int32_t                            count,
                 ywLong                            *val) {
    int32_t    i;
    for (i = 0; i < count; ++i) {
        assert(btree->insert(val[i], &val[i]));
    }
    for (i = 0; i < count; ++i) {
        if (btree->find(val[i]) != &val[i]) {
            assert(btree->find(val[i]) == &val[i]);
        }
    }

    for (i = 0; i < count; ++i) {
        assert(btree->remove(val[i]));
        if (btree->find(val[i]) == &val[i]) {
            assert(btree->find(val[i]) != &val[i]);
        }
    }
}

void btree_basic_test() {
    static const int32_t                     count = 1000000;
    typedef ywBTree<ywLong, ywPtr, 2*KB>     btree_type;
    btree_type                               btree;
    ywLong                                  *val = new ywLong[count];
    int32_t                                  i;
    uint32_t                                 rnd = 1;

    for (i = 0; i < count; ++i) {
        val[i] = ywLong(i);
    }
    btree_basic<2*KB>(&btree, count, val);
    btree_basic<2*KB>(&btree, count, val);
    btree.reset();

    for (i = 0; i < count; ++i) {
        val[i] = ywLong(count-i);
    }
    btree_basic<2*KB>(&btree, count, val);
    btree_basic<2*KB>(&btree, count, val);
    btree.reset();

    for (i = 0; i < count; ++i) {
        val[i] = ywLong(rand_r(&rnd));
    }
    btree_basic<2*KB>(&btree, count, val);
    btree_basic<2*KB>(&btree, count, val);
    btree.reset();

    delete val;
}


typedef ywBTree<ywLong, ywPtr,  1*KB>     btree_1k;
class ywBtreeConcTest {
 public:
    static const int32_t TEST_COUNT = 1024*1024*4;

    void run();

    btree_1k *tree;
    int32_t   operation;
};

void stress_task(void *arg) {
    ywBtreeConcTest *tc = reinterpret_cast<ywBtreeConcTest*>(arg);

    tc->run();
}

void ywBtreeConcTest::run() {
    int32_t i;
    int32_t val;

    for (i = 0; i < TEST_COUNT; ++i) {
        val = operation * TEST_COUNT + i;
        if (operation < 0) {
            tree->insert(ywLong(val), NULL);
        } else {
            tree->find(ywLong(val));
        }
    }
}

void btree_conc_insert(int32_t thread_count) {
    ywWorkerPool   *tpool = ywWorkerPool::get_instance();
    ywBtreeConcTest tc[MAX_THREAD_COUNT];
    btree_1k        tree;
    int32_t         i;
    int32_t         val;

    for (i = 0; i < ywBtreeConcTest::TEST_COUNT; ++i) {
        val = i;
        tree.insert(ywLong(val), NULL);
    }
    for (i = 0; i < thread_count; ++i) {
        tc[i].operation = i;
        tc[i].tree      = &tree;
        assert(tpool->add_task(stress_task, &tc[i]));
    }
    tpool->wait_to_idle();
}
