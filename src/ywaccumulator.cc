/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywaccumulator.h>
#include <gtest/gtest.h>

template<> int32_t     ywAccumulator<int64_t, 0>::global_idx = 0;
template<> int32_t     ywAccumulator<int64_t, 1>::global_idx = 0;
template<> int32_t     ywAccumulator<int64_t, 2>::global_idx = 0;

template<>
int64_t *ywAccumulator<int64_t, 0>::root[ywAccumulator::CHUNK_COUNT]={};
template<>
int64_t *ywAccumulator<int64_t, 1>::root[ywAccumulator::CHUNK_COUNT]={};
template<>
int64_t *ywAccumulator<int64_t, 2>::root[ywAccumulator::CHUNK_COUNT]={};

template<> int32_t     ywAccumulator<int64_t, 0>::free_count = 0;
template<> int32_t     ywAccumulator<int64_t, 1>::free_count = 0;
template<> int32_t     ywAccumulator<int64_t, 2>::free_count = 0;

template<> ywQueueHead<int32_t, false> ywAccumulator<int64_t, 0>::free_list{};
template<> ywQueueHead<int32_t, false> ywAccumulator<int64_t, 1>::free_list{};
template<> ywQueueHead<int32_t, false> ywAccumulator<int64_t, 2>::free_list{};

template<> ywSpinLock  ywAccumulator<int64_t, 0>::g_acc_lock{};
template<> ywSpinLock  ywAccumulator<int64_t, 1>::g_acc_lock{};
template<> ywSpinLock  ywAccumulator<int64_t, 2>::g_acc_lock{};

const uint32_t TEST_COUNT = 1024*1024;

void accumulate_task(void *arg) {
    ywAccumulator<int64_t, 0> *acc =
        reinterpret_cast<ywAccumulator<int64_t, 0>*>(arg);
    uint32_t                i;

    for (i = 0; i < TEST_COUNT ; ++i) {
        acc->mutate(3);
        acc->mutate(-2);
    }
}

void accumulator_test() {
    ywAccumulator<int64_t, 0>  acc;
    ywWorkerPool  *tpool = ywWorkerPool::get_instance();
    int            processor_count = ywWorkerPool::get_processor_count();
    int            i;

    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(accumulate_task, &acc));
    }
    tpool->wait_to_idle();

    EXPECT_EQ(TEST_COUNT * processor_count, acc.sum());
}

void atomic_stat_task(void *arg) {
    ywAccumulator<int64_t, 1>  *acc =
        reinterpret_cast<ywAccumulator<int64_t, 1>*>(arg);
    uint32_t                i;

    for (i = 0; i < TEST_COUNT ; ++i) {
        acc->mutate(3);
        acc->mutate(-2);
    }
}

void atomic_stat_test() {
    ywAccumulator<int64_t, 1>  acc;
    ywWorkerPool  *tpool = ywWorkerPool::get_instance();
    int            processor_count = ywWorkerPool::get_processor_count();
    int            i;

    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(atomic_stat_task, &acc));
    }
    tpool->wait_to_idle();

    EXPECT_EQ(TEST_COUNT * processor_count, acc.sum());
}

void dirty_stat_task(void *arg) {
    ywAccumulator<int64_t, 2>  *acc =
        reinterpret_cast<ywAccumulator<int64_t, 2>*>(arg);
    uint32_t                i;

    for (i = 0; i < TEST_COUNT ; ++i) {
        acc->mutate(3);
        acc->mutate(-2);
    }
}

void dirty_stat_test() {
    ywAccumulator<int64_t, 2>  acc;
    ywWorkerPool  *tpool = ywWorkerPool::get_instance();
    int            processor_count = ywWorkerPool::get_processor_count();
    int            i;

    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(dirty_stat_task, &acc));
    }
    tpool->wait_to_idle();
}
