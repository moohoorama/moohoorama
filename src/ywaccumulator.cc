/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywaccumulator.h>
#include <gtest/gtest.h>

const uint32_t TEST_COUNT = 1024*1024*32;

void accumulate_task(void *arg) {
    ywAccumulator<int32_t, false> *acc =
        reinterpret_cast<ywAccumulator<int32_t, false>*>(arg);
    uint32_t                i;

    for (i = 0; i < TEST_COUNT ; ++i) {
        acc->mutate(3);
        acc->mutate(-2);
    }
}

void accumulator_test() {
    ywThreadPool           *tpool = ywThreadPool::get_instance();
    int                     processor_count =
                                ywThreadPool::get_processor_count();
    ywAccumulator<int32_t, false>  acc;
    int                     i;

    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(accumulate_task, &acc));
    }
    tpool->wait_to_idle();

    EXPECT_EQ(TEST_COUNT * processor_count, acc.get());
}

void atomic_stat_task(void *arg) {
    int32_t  *acc = reinterpret_cast<int32_t*>(arg);
    uint32_t  i;

    for (i = 0; i < TEST_COUNT ; ++i) {
        __sync_add_and_fetch(acc, 3);
        __sync_add_and_fetch(acc, -2);
    }
}


void atomic_stat_test() {
    ywThreadPool           *tpool = ywThreadPool::get_instance();
    int                     processor_count =
                                ywThreadPool::get_processor_count();
    int                     acc = 0;
    int                     i;

    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(atomic_stat_task, &acc));
    }
    tpool->wait_to_idle();

    EXPECT_EQ(TEST_COUNT * processor_count, acc);
}
