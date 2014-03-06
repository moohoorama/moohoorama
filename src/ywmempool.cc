/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywmempool.h>
#include <gtest/gtest.h>

typedef struct {
    int64_t cal[4];
} test_t;

void mempool_basic_test() {
    ywMemPool<test_t>  pool;
    test_t            *test_slot[16];
    int                i;
    int                j;

    for (j = 0; j < 64; ++j) {
        for (i = 0; i < 16; ++i) {
            test_slot[i] = pool.alloc();
            EXPECT_TRUE(NULL !=  test_slot[i]);
        }
        for (i = 0; i < 16; ++i) {
            pool.free_mem(test_slot[i]);
        }
    }
    EXPECT_EQ(64*KB, pool.get_size());
    EXPECT_EQ(1, pool.get_chunk_count());
}

void mempool_guard_commit(ywMemPool<test_t> *pool, test_t *test_slot[16]) {
    ywMemPoolGuard<test_t> guard(pool);
    int                     i;

    for (i = 0; i < 16; ++i) {
        test_slot[i] = guard.alloc();
        EXPECT_TRUE(NULL !=  test_slot[i]);
    }
    guard.commit();
}

void mempool_guard_rollback(ywMemPool<test_t> *pool, test_t *test_slot[16]) {
    ywMemPoolGuard<test_t> guard(pool);
    int                     i;

    for (i = 0; i < 16; ++i) {
        test_slot[i] = guard.alloc();
        EXPECT_TRUE(NULL !=  test_slot[i]);
    }
}


void mempool_guard_test() {
    ywMemPool<test_t>  pool;
    test_t            *test_slot[16];
    int                i;
    int                j;

    for (j = 0; j < 64; ++j) {
        mempool_guard_commit(&pool, test_slot);
        for (i = 0; i < 16; ++i) {
            pool.free_mem(test_slot[i]);
        }
        mempool_guard_rollback(&pool, test_slot);
    }
    EXPECT_EQ(64*KB, pool.get_size());
    EXPECT_EQ(1, pool.get_chunk_count());
}
