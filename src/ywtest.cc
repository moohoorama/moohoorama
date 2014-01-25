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
#include <gtest/gtest.h>

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



int main(int argc, char ** argv) {
    ywGlobalInit();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
