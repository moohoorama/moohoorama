/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <stdio.h>
#include <assert.h>
#include <gtest/gtest.h>
#include <stdlib.h>

#include <ywdlist.h>
#include <ywq.h>
#include <ywutil.h>
#include <ywserializer.h>
#include <ywmain.h>
#include <ywspinlock.h>
#include <ywsl.h>
#include <ywskiplist.h>
#include <ywthread.h>
#include <ywaccumulator.h>
#include <ywrbtree.h>
#include <ywmempool.h>
#include <ywbtree.h>
#include <ywfbtree.h>
#include <ywrcuref.h>

#include <map>
#include <vector>
#include <algorithm>

TEST(ThreadPool, Basic) {
    int32_t i;
    for (i = 0; i < 4; ++i) {
        threadpool_test();
        printf("%d...done\n", i);
    }
}

TEST(Queue, Basic) {
    int32_t i;
    for (i = 0; i < 4; ++i) {
        ywq_test();
        printf("%d...done\n", i);
    }
}

TEST(RCURef, Basic) {
    rcu_ref_test();
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
    const int  THREAD_COUNT = 32;
    int        count = 0;
    int        i;

    for (i = 0; i< THREAD_COUNT; ++i) {
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                spin_routine, reinterpret_cast<void*>(&count)));
    }
    ywThreadPool::get_instance()->wait_to_idle();

    ASSERT_EQ(THREAD_COUNT * 65536, count);
}

TEST(Atomic, Count) {
    const int  THREAD_COUNT = 32;
    int        count = 0;
    int        i;

    for (i = 0; i< THREAD_COUNT; ++i) {
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                add_routine, reinterpret_cast<void*>(&count)));
    }
    ywThreadPool::get_instance()->wait_to_idle();

    ASSERT_EQ(THREAD_COUNT * 65536, count);
}

TEST(SyncList, Basic) {
    ywSyncList   list;
    ywNode       node[8];
    int          i;

    for (i = 0; i < 8; ++i) {
        ASSERT_TRUE(list.add(&node[i]));
    }
    for (i = 0; i < 8; ++i) {
        ASSERT_TRUE(list.remove(&node[i]));
    }
}

const int    THREAD_COUNT = 32;
ywSyncList   list;
ywNode       node[THREAD_COUNT][8192];

void sl_test_routine(void * arg) {
    int  num = reinterpret_cast<int>(arg);
    int  i;

    for (i = 0; i < 8192; ++i) {
        node[num][i].data = reinterpret_cast<void*>(num*100000+i);
        while (!list.add(&node[num][i])) {
        }
    }
    for (i = 0; i < 8192; ++i) {
        while (!list.remove(&node[num][i])) {
        }
    }
}


TEST(SyncList, Concurrency) {
    int        i;

    for (i = 0; i< THREAD_COUNT; ++i) {
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                sl_test_routine, reinterpret_cast<void*>(i)));
    }
    ywThreadPool::get_instance()->wait_to_idle();
}

TEST(Atomic, BasicPerformance) {
    atomic_stat_test();
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

/*
TEST(SkipList, ConcurrentInsert) {
    skiplist_conc_insert_test();
}
TEST(SkipList, ConcurrentRemove) {
    skiplist_conc_remove_test();
}
*/

static const int32_t DATA_SIZE = 1024*1024;
static const int32_t TRY_COUNT = 1024*1024*8;
int32_t data[DATA_SIZE];

typedef bool (*compareFunc)(int32_t a, int32_t b);
bool compare_int(int32_t a, int32_t b) {
    return a < b;
}

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


TEST(BinarySearch, Branch_func_pointer) {
    int32_t i;
    int32_t rnd = 2;
    int32_t val;
    int32_t min;
    int32_t max;
    int32_t mid;
    int32_t ret = 0;
    compareFunc func = compare_int;

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
            if (func(val, data[mid])) {
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
        }
    }
    fb_report(fbt);
}

TEST(FBTree, Basic) {
    fb_basic_test();
}

TEST(FBTree, Concurrent_insert) {
    fb_conc_test(DATA_SIZE, TRY_COUNT, 0);
}

TEST(FBTree, Concurrent_search) {
    fb_conc_test(DATA_SIZE, TRY_COUNT, 1);
}

TEST(FBTree, Concurrent_fusion) {
    fb_conc_test(DATA_SIZE, TRY_COUNT, 2);
}
TEST(FBTree, Concurrent_remove) {
    fb_conc_test(DATA_SIZE, TRY_COUNT, 3);
}


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


TEST(SkipList, LevelTest) {
    skiplist_level_test();
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

TEST(MemPool, Basic) {
    ywMemPool<int64_t> test;

    test.free_mem(test.alloc());
}

TEST(DoublyLinkedList, Basic) {
    ywdl_test();
}

int main(int argc, char ** argv) {
    ywGlobalInit();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
