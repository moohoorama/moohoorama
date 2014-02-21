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
#include <math.h>

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
        assert(0 == pthread_create(
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

static const uint32_t DATA_SIZE = 1024*1024*4;
#define MIN cursor[0]
#define MAX cursor[1]
#define MID cursor[2]

#include <vector>
#include <algorithm>

TEST(BinarySearch, BranchMethod) {
    std::vector<uint32_t> data(DATA_SIZE);
    uint32_t i;
    uint32_t size;
    uint32_t min, mid, max;
    uint32_t compare = 0;
    uint32_t rnd = 2;

    /*insertData*/
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = rand_r(&rnd) % DATA_SIZE;
        data[i]=rnd;
    }
    std::sort(data.begin(), data.end());
    rnd = 2;
    size = DATA_SIZE;

    for (i = 0; i < DATA_SIZE; ++i) {
        min = 0;
        max = size-1;
        rnd = rand_r(&rnd) % DATA_SIZE;

        do {
            compare++;
            mid = (min + max)/2;
            if (rnd < data[mid]) {
                max = mid - 1;
            } else {
                min = mid + 1;
            }
        } while (min <= max);
//        printf("%12d %12d %12d\n", data[mid], rnd, data[mid+1]);
    }
    printf("Compare:%d\n", compare);
}


TEST(BinarySearch, NoBranchMethod) {
//    uint32_t data[DATA_SIZE];
    std::vector<uint32_t> data(DATA_SIZE);
    uint32_t i;
    uint32_t size;
    uint32_t j;
    uint32_t log_size;
    register uint32_t cursor[3];
    uint32_t sign;
    uint32_t compare = 0;
    uint32_t rnd = 2;

    /*insertData*/
    for (i = 0; i < DATA_SIZE; ++i) {
        rnd = rand_r(&rnd) % DATA_SIZE;
        data[i]=rnd;
    }
    std::sort(data.begin(), data.end());
    rnd = 2;

    size = DATA_SIZE;
    log_size = ffsl(i)-1;
//    printf("%d,%d\n", size, log_size);
    for (i = 0; i < DATA_SIZE; ++i) {
        MIN = 0;
        MAX = size-1;
        rnd = rand_r(&rnd) % DATA_SIZE;

        for (j = 0; j <= log_size; ++j) {
//        do {
            compare++;
            MID = (MIN + MAX)/2;
//            sign = (rnd < data[MID]);
            sign = (rnd - data[MID])>>31;
            cursor[ sign ] = MID - (sign*2-1);
        }
//        printf("%12d %12d %12d\n", data[MID], rnd, data[MID+1]);
//        } while (MIN <= MAX);
    }
    printf("Compare:%d\n", compare);
}


int main(int argc, char ** argv) {
    ywGlobalInit();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
