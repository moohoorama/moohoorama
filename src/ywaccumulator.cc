/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywaccumulator.h>
#include <gtest/gtest.h>

ywSpinLock  g_acc_lock;
template<>
int32_t     ywAccumulator<int64_t, true>::global_idx = 0;
template<>
int64_t    *ywAccumulator<int64_t, true>::root[ywAccumulator::CHUNK_COUNT]={};
template<>
int32_t     ywAccumulator<int64_t, true>::free_count = 0;
template<>
ywsllNode   ywAccumulator<int64_t, true>::free_list{};

template<>
int32_t     ywAccumulator<int64_t, false>::global_idx = 0;
template<>
int64_t    *ywAccumulator<int64_t, false>::root[ywAccumulator::CHUNK_COUNT]={};
template<>
int32_t     ywAccumulator<int64_t, false>::free_count = 0;
template<>
ywsllNode   ywAccumulator<int64_t, false>::free_list{};


template<>
bool ywAccumulator<int64_t, false>::lock() {
    return g_acc_lock.WLock();
}

template<>
void ywAccumulator<int64_t, false>::release() {
    g_acc_lock.release();
}

template<>
bool ywAccumulator<int64_t, true>::lock() {
    return g_acc_lock.WLock();
}

template<>
void ywAccumulator<int64_t, true>::release() {
    g_acc_lock.release();
}

const uint32_t TEST_COUNT = 1024*1024;

void accumulate_task(void *arg) {
    ywAccumulator<int64_t, false> *acc =
        reinterpret_cast<ywAccumulator<int64_t, false>*>(arg);
    uint32_t                i;

    for (i = 0; i < TEST_COUNT ; ++i) {
        acc->mutate(3);
        acc->mutate(-2);
    }
}

void accumulator_test() {
    ywAccumulator<int64_t, false>  acc;
    ywThreadPool  *tpool = ywThreadPool::get_instance();
    int            processor_count = ywThreadPool::get_processor_count();
    int            i;

    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(accumulate_task, &acc));
    }
    tpool->wait_to_idle();

    EXPECT_EQ(TEST_COUNT * processor_count, acc.get());
}

void atomic_stat_task(void *arg) {
    ywAccumulator<int64_t, true>  *acc =
        reinterpret_cast<ywAccumulator<int64_t, true>*>(arg);
    uint32_t                i;

    for (i = 0; i < TEST_COUNT ; ++i) {
        acc->mutate(3);
        acc->mutate(-2);
    }
}

void atomic_stat_test() {
    ywAccumulator<int64_t, true>  acc;
    ywThreadPool  *tpool = ywThreadPool::get_instance();
    int            processor_count = ywThreadPool::get_processor_count();
    int            i;

    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(atomic_stat_task, &acc));
    }
    tpool->wait_to_idle();

    EXPECT_EQ(TEST_COUNT * processor_count, acc.get());
}
