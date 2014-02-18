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

#include <map>

#ifdef DEBUG
TEST(Queue, Basic) {
    ywqTest();
}

TEST(Serialization, Basic) {
    EXPECT_TRUE(ywsTest());
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

    EXPECT_TRUE(spin_lock.tryWLock());
    EXPECT_FALSE(spin_lock.tryWLock());
    EXPECT_FALSE(spin_lock.tryRLock());
    spin_lock.release();

    EXPECT_TRUE(spin_lock.tryRLock());
    EXPECT_TRUE(spin_lock.tryRLock());
    EXPECT_FALSE(spin_lock.tryWLock());
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

    EXPECT_EQ(THREAD_COUNT * 65536, count);
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

    EXPECT_EQ(THREAD_COUNT * 65536, count);
}

TEST(SyncList, Basic) {
    ywSyncList   list;
    ywNode       node[8];
    int          i;

    list.print();
    for (i = 0; i < 8; ++i) {
        EXPECT_TRUE(list.add(&node[i]));
        list.print();
    }
    for (i = 0; i < 8; ++i) {
        EXPECT_TRUE(list.remove(&node[i]));
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
#endif

TEST(SkipList, LevelTest) {
    uint32_t   stat[ywSkipList::MAX_LEVEL] = {0};
    uint32_t   i;
    uint32_t   j;
    uint32_t   degree;
    uint32_t   val;
    uint32_t   expect;
    static const uint32_t TEST_COUNT = 1024*16;
    for (j = 1; j < ywSkipList::MAX_LEVEL; ++j) {
        ywSkipList yw_skip_list(j);

        memset(&stat, 0, sizeof(stat));

        printf("Level:%d\n", j);
        for (i = 0; i < TEST_COUNT; ++i) {
            stat[ yw_skip_list.get_new_level() ]++;
        }
        degree = yw_skip_list.getDegree();
        val = TEST_COUNT;
        for (i = 1; i <= j; ++i) {
            if (i == j || val < degree) {
                break;
            } else {
                expect = val * (degree-1)/degree;
                EXPECT_EQ(expect, stat[i]);
                val -= expect;
            }
            printf("%8d : %8d\n", i, stat[i]);
        }
    }
}
TEST(SkipList, InsertDeleteTest) {
    ywSkipList yw_skip_list;
    uint32_t   i;

    for (i = 1; i < 4096; ++i) {
        EXPECT_TRUE(yw_skip_list.insert(i));
    }

    for (i = 1; i < 4096; ++i) {
        EXPECT_TRUE(yw_skip_list.remove(i));
    }
    yw_skip_list.print();
}
TEST(SkipList, InsertPerformance) {
    ywSkipList yw_skip_list;
    uint32_t   rnd;
    uint32_t   i;

    rnd = 3;
    for (i = 0; i < 1024*1024*16; ++i) {
        rnd = rand_r(&rnd);
        EXPECT_TRUE(yw_skip_list.insert(rnd));
    }
}
TEST(Map, InsertPerformance) {
    std::map<uint32_t, uint32_t> test_map;
    uint32_t                rnd;
    uint32_t                i;

    rnd = 3;
    for (i = 0; i < 1024*1024*16; ++i) {
        rnd = rand_r(&rnd);
        test_map[rnd]=rnd;
    }
}

int main(int argc, char ** argv) {
    ywGlobalInit();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
