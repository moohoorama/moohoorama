/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include <ywdlist.h>
#include <ywq.h>
#include <ywutil.h>
#include <ywserializer.h>
#include <ywmain.h>
#include <ywspinlock.h>
#include <ywsl.h>
#include <ywskiplist.h>
#include <gtest/gtest.h>
#include <ywthread.h>
#include <ywaccumulator.h>
#include <stdlib.h>
#include <ywrbtree.h>
#include <vector>
#include <algorithm>


#ifdef __DEBUG
TEST(Queue, Basic) {
    ywqTest();
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

void * add_routine(void * arg) {
    int *var = reinterpret_cast<int*>(arg);
    int  i;

    for (i = 0; i < 65536; ++i) {
        __sync_fetch_and_add(var, 1);
    }

    return NULL;
}

void * spin_routine(void * arg) {
    int *var = reinterpret_cast<int*>(arg);
    int  i;

    for (i = 0; i < 65536; ++i) {
        while (!global_spin_lock.WLock()) {
        }
        (*var)++;
        global_spin_lock.release();
    }

    return NULL;
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
    pthread_t  pt[THREAD_COUNT];
    int        count = 0;
    int        i;

    for (i = 0; i< THREAD_COUNT; ++i) {
        assert(0 == pthread_create(
                    &pt[i], NULL/*attr*/, spin_routine,
                    reinterpret_cast<void*>(&count)));
    }
    for (i = 0; i< THREAD_COUNT; ++i)
        pthread_join(pt[i], NULL);

    ASSERT_EQ(THREAD_COUNT * 65536, count);
}

TEST(Atomic, Count) {
    const int  THREAD_COUNT = 32;
    pthread_t  pt[THREAD_COUNT];
    int        count = 0;
    int        i;

    for (i = 0; i< THREAD_COUNT; ++i) {
        assert(0 == pthread_create(
                    &pt[i], NULL/*attr*/, add_routine,
                    reinterpret_cast<void*>(&count)));
    }
    for (i = 0; i< THREAD_COUNT; ++i)
        pthread_join(pt[i], NULL);

    ASSERT_EQ(THREAD_COUNT * 65536, count);
}

TEST(SyncList, Basic) {
    ywSyncList   list;
    ywNode       node[8];
    int          i;

    list.print();
    for (i = 0; i < 8; ++i) {
        ASSERT_TRUE(list.add(&node[i]));
        list.print();
    }
    for (i = 0; i < 8; ++i) {
        ASSERT_TRUE(list.remove(&node[i]));
        list.print();
    }
}

const int    THREAD_COUNT = 32;
ywSyncList   list;
ywNode       node[THREAD_COUNT][8192];

void * sl_test_routine(void * arg) {
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

    return NULL;
}


TEST(SyncList, Concurrency) {
    pthread_t  pt[THREAD_COUNT];
    int        i;

    for (i = 0; i< THREAD_COUNT; ++i) {
        ASSERT_EQ(0, pthread_create(
                    &pt[i], NULL, sl_test_routine,
                    reinterpret_cast<void*>(i)));
    }
    for (i = 0; i< THREAD_COUNT; ++i)
        pthread_join(pt[i], NULL);

    list.print();
}

TEST(ThreadPool, Basic) {
    threadpool_test();
}

TEST(Atomic, BasicPerformance) {
    atomic_stat_test();
}
TEST(Accumulator, BasicPerformance) {
    accumulator_test();
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

TEST(SkipList, ConcurrentInsert) {
    skiplist_conc_insert_test();
}
TEST(SkipList, ConcurrentRemove) {
    skiplist_conc_remove_test();
}
#endif

static const int32_t DATA_SIZE = 1024*64;
static const int32_t TRY_COUNT = 16;
int32_t data[DATA_SIZE];

typedef bool (*compareFunc)(int32_t a, int32_t b);
bool compare_int(int32_t a, int32_t b) {
    return a < b;
}

static node_t *root = rb_create_tree();

TEST(RBTree, Generate) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    rnd = 2;
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
        rb_insert(&root, rnd % DATA_SIZE*4);
        val = rnd % DATA_SIZE*4;
        if (val) {
            rb_insert(&root, val);
        }
    }
    rb_validation(root);
}

TEST(RBTree, Search) {
    int32_t rnd = 2;
    int32_t i;
    int32_t j;
    int32_t val;

    for (j = 0; j < TRY_COUNT; ++j) {
        for (i = 0; i < DATA_SIZE; ++i) {
            rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
            val = rnd % DATA_SIZE*4;
            rb_find(&root, val);
        }
    }
    printf("CompareCount : %d\n", rb_get_compare_count());
    printf("try_count    : %d\n", TRY_COUNT*DATA_SIZE);
    printf("AVG_COMPARE  : %d\n", rb_get_compare_count()/TRY_COUNT/DATA_SIZE);
}
TEST(RBTree, Remove) {
    int32_t rnd = 2;
    int32_t i;
    int32_t val;

    rnd = 2;
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
        val = rnd % DATA_SIZE*4;
        if (val) {
            rb_remove(&root, val);
        }
    }
}

TEST(BinarySearch, GenerateData) {
    int32_t i;
    int32_t rnd = 2;

    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
        data[i] = rnd % DATA_SIZE*4;
    }
    std::sort(&data[0], &data[DATA_SIZE-1]);
    data[0] = 0;
    data[DATA_SIZE-1] = INT32_MAX;
}

TEST(BinarySearch, std) {
    int32_t i;
    int32_t j;
    int32_t rnd = 2;
    bool    ret;

    for (j = 0; j < TRY_COUNT; ++j) {
        for (i = 0; i < DATA_SIZE; ++i) {
            rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
            ret = std::binary_search(&data[0], &data[DATA_SIZE-1],
                    rnd % DATA_SIZE*4);
        }
    }
    printf("%d\n", ret);
}

TEST(BinarySearch, Branch) {
    int32_t i;
    int32_t j;
    int32_t rnd = 2;
    int32_t val;
    int32_t min;
    int32_t max;
    int32_t mid;

    for (j = 0; j < TRY_COUNT; ++j) {
        for (i = 0; i < DATA_SIZE; ++i) {
            rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
            val = rnd % DATA_SIZE*4;

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

            if (!((data[mid] <= val) && (val <= data[mid+1]))) {
                printf("[%d]=%d <> %d <> %d\n",
                        mid, data[mid], val, data[mid+1]);
                ASSERT_TRUE(false);
            }
        }
    }
}


TEST(BinarySearch, Branch_func_pointer) {
    int32_t i;
    int32_t j;
    int32_t rnd = 2;
    int32_t val;
    int32_t min;
    int32_t max;
    int32_t mid;
    compareFunc func = compare_int;

    for (j = 0; j < TRY_COUNT; ++j) {
        for (i = 0; i < DATA_SIZE; ++i) {
            rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
            val = rnd % DATA_SIZE*4;

            min = 0;
            max = DATA_SIZE-1;
            do {
                mid = (min+max)/2;
                // if (val < data[mid]) {
                if (func(val, data[mid])) {
                    max = mid - 1;
                } else {
                    min = mid + 1;
                }
            } while (min <= max);
            mid = (min+max)/2;

            if (!((data[mid] <= val) && (val <= data[mid+1]))) {
                printf("[%d]=%d <> %d <> %d\n",
                        mid, data[mid], val, data[mid+1]);
                ASSERT_TRUE(false);
            }
        }
    }
}

TEST(BinarySearch, NoBranch) {
    int32_t i;
    int32_t j;
    int32_t rnd = 2;
    int32_t val;
    int32_t size;
    int32_t sign;
    int32_t idx;
    compareFunc func = compare_int;

    for (j = 0; j < TRY_COUNT; ++j) {
        for (i = 0; i < DATA_SIZE; ++i) {
            rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
            val = rnd % DATA_SIZE*4;

            size = DATA_SIZE;
            idx = 0;
            do {
                // sign = val >= data[size * idx + (size >> 1)];
                sign = !func(val, data[size * idx + (size >> 1)]);
                idx  = (idx << 1) + (sign);
                size >>= 1;
            } while (size > 1);


            if (!((data[idx] <= val) && (val <= data[idx+1]))) {
                printf("[%d]=%d <> %d <> %d\n",
                        idx, data[idx], val, data[idx+1]);
                ASSERT_TRUE(false);
            }
        }
    }
}

TEST(BinarySearch, NoBranch2) {
    int32_t i;
    int32_t j;
    int32_t rnd = 2;
    int32_t idx;
    int32_t size;
    int32_t val;
    compareFunc func = compare_int;

    for (j = 0; j < TRY_COUNT; ++j) {
        for (i = 0; i < DATA_SIZE; ++i) {
            rnd = rand_r(reinterpret_cast<uint32_t*>(&rnd));
            val = rnd % DATA_SIZE*4;

            idx  = 0;
            size   = DATA_SIZE;
            do {
                size >>= 1;
                idx += size*(!func(val, data[idx + size]));
            } while (size > 1 );

            if (!((data[idx] <= val) && (val <= data[idx+1]))) {
                printf("%d [%d]=%d <> %d <> %d\n",
                        i, idx, data[idx], val, data[idx+1]);
                ASSERT_TRUE(false);
            }
        }
    }
}



int main(int argc, char ** argv) {
    ywGlobalInit();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
