/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrcuref.h>
#include <gtest/gtest.h>
#include <ywsll.h>

ywrcu_free_queue             ywRcuRef::free_q;
ywMemPool<ywrcu_free_queue>  ywRcuRef::rc_free_pool;
volatile ywRcuRef            ywRcuRef::refs[REF_COUNT];
volatile ywRcuRef            ywRcuRef::global_refs;

ywAccumulator<int64_t>       rem_count;

static const int32_t TEST_COUNT = 1024*1024;
static const int32_t SLOT_COUNT = 1024;
static const int32_t magic = 0x1234;
static int32_t       slots[MAX_THREAD_COUNT][SLOT_COUNT];

void        *ptr;
ywsllNode    free_list;
ywSpinLock   free_lock;

void aging() {
    volatile int32_t   *int_ptr;
    void               *freeable;

    while (!(freeable = ywRcuRef::get_freeable_item())) {
        usleep(100);
    }

    int_ptr = reinterpret_cast<volatile int32_t*>(freeable);
    while (!free_lock.WLock()) {
    }
    ywsll_attach(&free_list, reinterpret_cast<ywsllNode*>(
            const_cast<int32_t*>(int_ptr)));
    free_lock.release();
    rem_count.mutate(1);
}
void rcu_ref_task(void *arg) {
    int32_t             tid = reinterpret_cast<int32_t>(arg);
    volatile int32_t   *int_ptr;
    void               *freeable;
    int32_t             i;

    for (i = 0; i < TEST_COUNT; ++i) {
        if (i < SLOT_COUNT) {
            slots[tid][i % SLOT_COUNT] = magic;
            ywRcuRef::set(&ptr, &slots[tid][i]);
        } else {
            do {
                while (!free_lock.WLock()) {
                }
                int_ptr = reinterpret_cast<volatile int32_t*>
                    (ywsll_pop(&free_list));
                free_lock.release();
                if (!int_ptr) {
                    aging();
                }
            } while (!int_ptr);
            *int_ptr = magic;
            freeable = reinterpret_cast<void*>(
                const_cast<int32_t*>(int_ptr));
            ywRcuRef::set(&ptr, freeable);
        }

        int_ptr = reinterpret_cast<volatile int32_t*>(ywRcuRef::fix(&ptr));
        if (*int_ptr != magic) {
            printf("idx : %d\n", ywRcuRef::get_slot_idx(&ptr));
            printf("slot: %lld\n", ywRcuRef::get_slot(&ptr));
            printf("fix : %lld\n", ywRcuRef::get_fix_slot(&ptr));
            assert(false);
        }
        ywRcuRef::unfix(&ptr);
    }
}

void rcu_ref_test() {
    ywThreadPool  *tpool = ywThreadPool::get_instance();
    int            processor_count = ywThreadPool::get_processor_count();
    int            i;
    int            init = magic;
    int            test = magic;

    ptr = &init;
    ywRcuRef::set(&ptr, &test);

//    processor_count = 1;
    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(rcu_ref_task,
                    reinterpret_cast<void*>(i)));
    }
    tpool->wait_to_idle();
    assert(rem_count.get() == (TEST_COUNT-SLOT_COUNT)*processor_count);
    assert(ywRcuRef::get_free_count() == SLOT_COUNT*processor_count + 1);
}


