/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <stdio.h>
#include <assert.h>
#include <gtest/gtest.h>
#include <stdlib.h>

#include <ywq.h>
#include <ywutil.h>
#include <ywserializer.h>
#include <ywmain.h>
#include <ywspinlock.h>
#include <ywsl.h>
#include <ywskiplist.h>
#include <ywworker.h>
#include <ywaccumulator.h>
#include <ywrbtree.h>
#include <ywmempool.h>
#include <ywfbtree.h>
#include <ywrcuref.h>
#include <yworderednode.h>
#include <ywpool.h>
#include <ywstack.h>
#include <ywbtree.h>
#include <ywbarray.h>
#include <ywkey.h>
#include <ywfsnode.h>

#include <map>
#include <vector>
#include <algorithm>

TEST(FSNode, Basic) {
    FSNode_basic_test();
}

TEST(Btree, Basic) {
    btree_basic_test();
}

TEST(Btree, conc_insert1) {
    btree_conc_insert(1);
}
TEST(Btree, conc_insert2) {
    btree_conc_insert(2);
}
TEST(Btree, conc_insert4) {
    btree_conc_insert(4);
}
TEST(Btree, conc_insert8) {
    btree_conc_insert(8);
}

TEST(Barray, Basic) {
    barray_test();
}

TEST(Key, Basic) {
    key_test();
}

TEST(OrderedNode, Basic) {
    OrderedNode_basic_test();
}

TEST(OrderedNode, Concurrency) {
    OrderedNode_bsearch_insert_conc_test();
}

TEST(OrderedNode, Stress1) {
    OrderedNode_stress_test(1);
}

TEST(OrderedNode, Stress2) {
    OrderedNode_stress_test(2);
}

TEST(OrderedNode, Stress4) {
    OrderedNode_stress_test(4);
}

TEST(OrderedNode, Stress8) {
    OrderedNode_stress_test(8);
}

TEST(OrderedNode, Stress16) {
    OrderedNode_stress_test(16);
}

TEST(OrderedNode, Basic_huge_binary) {
    OrderedNode_search_test(1024, 0);
}

TEST(OrderedNode, search_huge_interpolation) {
    OrderedNode_search_test(1024, 1);
}

TEST(RCURef, Basic) {
    rcu_ref_test();
}

TEST(ywStack, Basic) {
    stack_basic_test();
}

TEST(FBTree, Basic) {
    fb_basic_test();
}

TEST(Accumulator, method_0_array) {
    accumulator_test();
}
TEST(Accumulator, method_1_atomic) {
    atomic_stat_test();
}
TEST(Accumulator, method_2_dirty) {
    dirty_stat_test();
}

TEST(Pool, Basic) {
    ywPoolTest();
}

TEST(PoolCCTest, Single) {
    ywPoolCCTest(1);
}
TEST(PoolCCTest, 2) {
    ywPoolCCTest(2);
}
TEST(PoolCCTest, 4) {
    ywPoolCCTest(4);
}
TEST(PoolCCTest, 8) {
    ywPoolCCTest(8);
}

TEST(Queue, SyncPerf) {
    const int32_t              TRY_COUNT  = 1024*64;
    const int32_t              ITER_COUNT = 64;
    ywQueueHead<int32_t, true> head;
    ywQueue<int32_t>           slot[TRY_COUNT];
    int                        i;
    int                        j;

    for (j = 0; j < ITER_COUNT; ++j) {
        for (i = 0; i < TRY_COUNT; ++i) {
            head.push(&slot[i]);
        }
        ASSERT_EQ(TRY_COUNT, head.calc_count());
        for (i = 0; i < TRY_COUNT; ++i) {
            head.pop();
        }
        ASSERT_EQ(0, head.calc_count());
    }
}

TEST(Queue, UnsyncPerf) {
    const int32_t               TRY_COUNT  = 1024*64;
    const int32_t               ITER_COUNT = 64;
    ywQueueHead<int32_t, false> head;
    ywQueue<int32_t>            slot[TRY_COUNT];
    int                         i;
    int                         j;

    for (j = 0; j < ITER_COUNT; ++j) {
        for (i = 0; i < TRY_COUNT; ++i) {
            head.push(&slot[i]);
        }
        ASSERT_EQ(TRY_COUNT, head.calc_count());
        for (i = 0; i < TRY_COUNT; ++i) {
            head.pop();
        }
        ASSERT_EQ(0, head.calc_count());
    }
}

TEST(Queue, Basic) {
    int32_t i;
    for (i = 0; i < 1; ++i) {
        ywq_test();
    }
}

static const int32_t DATA_SIZE = 1024*1024;
static const int32_t TRY_COUNT = 1024*1024*8;
int32_t data[DATA_SIZE];

int32_t get_next_rnd(int32_t prev) {
//    return simple_hash(sizeof(prev), reinterpret_cast<char*>(&prev))
//        % INT32_MAX;
//    return prev + 1;
    if (prev == 1) {
        return INT32_MAX - 16;
    }
    return prev - 1;
//    return rand_r(reinterpret_cast<uint32_t*>(&prev));
//    return (prev+1) % 64;
}

void *fbt = fb_create();

TEST(FBTree, Generate) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;
    int32_t count = 0;

    rnd = 2;
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        if (val) {
            if (fb_insert(fbt, val, NULL)) {
                count++;
            }
        }
    }
    fb_report(fbt);
}

TEST(FBTree, Search) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    for (i = 0; i < TRY_COUNT; ++i) {
        if (i % DATA_SIZE == 0) {
            rnd = 2;
        }
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        fb_find(fbt, val);
    }
}

TEST(FBTree, Remove) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    rnd = 2;
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        if (val) {
            fb_remove(fbt, val);
//            fb_validation(fbt);
        }
    }
    fb_report(fbt);
}

TEST(ThreadPool, Basic) {
    int32_t i;
    for (i = 0; i < 8; ++i) {
        threadpool_test();
        printf("\r%d%%", i*100/8);
    }
    printf("\r%d%%\n", i*100/8);
}

TEST(Serialization, Basic) {
    ASSERT_TRUE(ywsTest());
}

TEST(Dump, Basic) {
    char test[]="ABCDEF";
    printHex(sizeof(test), test);
    dump_stack();
}

ywSpinLock global_spin_lock;

void add_routine(void * arg) {
    int *var = reinterpret_cast<int*>(arg);
    int  i;

    for (i = 0; i < 65536; ++i) {
        __sync_fetch_and_add(var, 1);
    }
}

void spin_routine(void * arg) {
    int *var = reinterpret_cast<int*>(arg);
    int  i;

    for (i = 0; i < 65536; ++i) {
        while (!global_spin_lock.WLock()) {
        }
        (*var)++;
        global_spin_lock.release();
    }
}

TEST(Spinlock, Basic) {
    ywSpinLock spin_lock;

    ASSERT_TRUE(spin_lock.tryWLock());
    ASSERT_FALSE(spin_lock.tryWLock());
    ASSERT_FALSE(spin_lock.tryRLock());
    spin_lock.release();

    ASSERT_TRUE(spin_lock.tryRLock());
    ASSERT_TRUE(spin_lock.tryRLock());
    ASSERT_FALSE(spin_lock.tryWLock());
    spin_lock.release();
}
TEST(Spinlock, Count) {
    const int  THREAD_COUNT = 4;
    int        count = 0;
    int        i;

    for (i = 0; i< THREAD_COUNT; ++i) {
        ASSERT_TRUE(ywWorkerPool::get_instance()->add_task(
                spin_routine, reinterpret_cast<void*>(&count)));
    }
    ywWorkerPool::get_instance()->wait_to_idle();

    ASSERT_EQ(THREAD_COUNT * 65536, count);
}

TEST(Atomic, Count) {
    const int  THREAD_COUNT = 4;
    int        count = 0;
    int        i;

    for (i = 0; i< THREAD_COUNT; ++i) {
        ASSERT_TRUE(ywWorkerPool::get_instance()->add_task(
                add_routine, reinterpret_cast<void*>(&count)));
    }
    ywWorkerPool::get_instance()->wait_to_idle();

    ASSERT_EQ(THREAD_COUNT * 65536, count);
}

TEST(MemPool, Basic) {
    mempool_basic_test();
}
TEST(MemPool, Guard) {
    mempool_guard_test();
}

/*
TEST(SkipList, ConcurrentInsert) {
    skiplist_conc_insert_test();
}
TEST(SkipList, ConcurrentRemove) {
    skiplist_conc_remove_test();
}
*/

TEST(BinarySearch, GenerateData) {
    int32_t i;
    int32_t rnd = 2;

    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = get_next_rnd(rnd);
        data[i] = rnd % INT32_MAX;
    }
    std::sort(&data[0], &data[DATA_SIZE]);
}

int32_t NoBranchSearch(int32_t key) {
    int32_t *idx;
    int32_t size;
    int32_t *mid;

    idx  = data;
    size = DATA_SIZE;
    do {
        size >>= 1;
        mid = idx+size;
        if (key >= *mid) {
            idx = mid;
        }
    } while (size >= 1);

    return *mid;
}


TEST(BinarySearch, NoBranch) {
    int32_t i;
    int32_t rnd = 2;
    int32_t val;

    for (i = 0; i < TRY_COUNT; ++i) {
        if (i % DATA_SIZE == 0) {
            rnd = 2;
        }
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;

        assert(NoBranchSearch(val) == val);
//        assert(NoBranchSearch(val) > 0);
    }
}


TEST(BinarySearch, std) {
    int32_t i;
    int32_t rnd = 2;
    int32_t ret = 0;

    for (i = 0; i < TRY_COUNT; ++i) {
        if (i % DATA_SIZE == 0) {
            rnd = 2;
        }
        rnd = get_next_rnd(rnd);
        ret = *std::lower_bound(&data[0], &data[DATA_SIZE],
                                 rnd % INT32_MAX);
        assert(ret == rnd);
    }
}

TEST(BinarySearch, Branch) {
    int32_t i;
    int32_t rnd = 2;
    int32_t val;
    int32_t min;
    int32_t max;
    int32_t mid;
    int32_t ret = 0;

    for (i = 0; i < TRY_COUNT; ++i) {
        if (i % DATA_SIZE == 0) {
            rnd = 2;
        }
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;

        min = 0;
        max = DATA_SIZE-1;
        do {
            mid = (min+max)/2;
            if (val < data[mid]) {
                max = mid - 1;
            } else {
                min = mid + 1;
            }
        } while (min <= max);
        mid = (min+max)/2;

        ret = data[mid];
        assert(ret == rnd);
    }
}

static void * rbt = rb_create_tree();

TEST(RBTree, Generate) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    rnd = 2;
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        if (val) {
            ASSERT_TRUE(rb_insert(rbt, val, NULL));
        }
    }
//    rb_validation(rbt);
}

TEST(RBTree, Search) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    for (i = 0; i < TRY_COUNT; ++i) {
        if (i % DATA_SIZE == 0) {
            rnd = 2;
        }
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        rb_find(rbt, val);
    }
    //    ASSERT_EQ(8872084, rb_get_compare_count());
}

/*
   TEST(RBTree, concurrency) {
   rbtree_concunrrency_test(rbt);
   }
 */

TEST(RBTree, Remove) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    rnd = 2;
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        if (val) {
            rb_remove(rbt, val);
        }
    }
//    rb_validation(rbt);
    rb_print(rbt);
}

/*
TEST(FBTree, Concurrent_insert) {
    fb_conc_test(DATA_SIZE, TRY_COUNT, 0);
}

TEST(FBTree, Concurrent_search) {
    fb_conc_test(DATA_SIZE, TRY_COUNT, 1);
}

TEST(FBTree, Concurrent_remove) {
    fb_conc_test(DATA_SIZE, TRY_COUNT, 2);
}

TEST(FBTree, Concurrent_fusion) {
    fb_conc_test(DATA_SIZE, TRY_COUNT, 3);
}
*/

std::map<int32_t, int32_t> test_map;

TEST(STD_MAP, Generate) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    rnd = 2;
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        if (val) {
            test_map[val] = val;
        }
    }
}

TEST(STD_MAP, Search) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    for (i = 0; i < TRY_COUNT; ++i) {
        if (i % DATA_SIZE == 0) {
            rnd = 2;
        }
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        if (val) {
            ASSERT_EQ(val, test_map[val]);
        }
    }
}

TEST(STD_MAP, Remove) {
    test_map.clear();
}

ywSkipList     test_skip_list;

TEST(SkipList, LevelTest) {
    skiplist_level_test();
}

TEST(SkipList, Generate) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    rnd = 2;
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        if (val) {
            test_skip_list.insert(val);
        }
    }
}

TEST(SkipList, Search) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    for (i = 0; i < TRY_COUNT; ++i) {
        if (i % DATA_SIZE == 0) {
            rnd = 2;
        }
        rnd = get_next_rnd(rnd);
        val = rnd % INT32_MAX;
        if (val) {
            ASSERT_EQ(val, (int32_t)test_skip_list.search(val)->key);
        }
    }
}
TEST(SkipList, Remove) {
}


TEST(SkipList, InsertRemove) {
    skiplist_insert_remove_test();
}

TEST(StdMap, InsertPerformance) {
    skiplist_map_insert_perf_test();
}

TEST(SkipList, InsertPerformance) {
    skiplist_insert_perf_test();
}

int main(int argc, char ** argv) {
    ywGlobalInit();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
