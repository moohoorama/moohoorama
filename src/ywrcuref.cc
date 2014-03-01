/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrcuref.h>
#include <gtest/gtest.h>

ywrcu_free_queue             ywRcuRef::free_q;
ywMemPool<ywrcu_free_queue>  ywRcuRef::rc_free_pool;
volatile ywRcuRef            ywRcuRef::refs[REF_COUNT];
volatile ywRcuRef            ywRcuRef::global_refs;

ywAccumulator<int64_t>       rem_count;

static const int32_t TEST_COUNT = 1024*1024;
static const int32_t SLOT_COUNT = 1024*1024;
static const int32_t magic = 0x1234;
static int32_t       slots[MAX_THREAD_COUNT][SLOT_COUNT];

void *ptr;

void rcu_ref_task(void *arg) {
    int32_t             tid = reinterpret_cast<int32_t>(arg);
    volatile int32_t   *int_ptr;
    ywrcu_free_queue   *freeable = NULL;
//    void               *freeable;
    int32_t             i;

    i = ywRcuRef::get_slot_idx(&ptr);
//    printf("idx : %d\n", i);

    for (i = 0; i < TEST_COUNT; ++i) {
        while (slots[tid][i % SLOT_COUNT] == magic) {
            printf("wait...\n");
            sleep(1);
        }
        slots[tid][i % SLOT_COUNT] = magic;
        ywRcuRef::set(&ptr, &slots[tid][i]);

        int_ptr = reinterpret_cast<volatile int32_t*>(ywRcuRef::fix(&ptr));
        if (*int_ptr != magic) {
            printf("idx : %d\n", ywRcuRef::get_slot_idx(&ptr));
            printf("slot: %lld\n", ywRcuRef::get_slot(&ptr));
            printf("fix : %lld\n", ywRcuRef::get_fix_slot(&ptr));
            assert(false);
        }
        ywRcuRef::unfix(&ptr);

        while ((freeable = ywRcuRef::_get_freeable_item())) {
            int_ptr =
                reinterpret_cast<volatile int32_t*>(freeable->data.target);
            if (*int_ptr != magic) {
                printf("slot : %lld\n", ywRcuRef::get_slot(&ptr));
                printf("fix  : %lld\n", ywRcuRef::get_fix_slot(&ptr));
                printf("_time: %lld\n", freeable->data.remove_time);
                printf("value: %d\n", *int_ptr);
                assert(false);
            } else {
                rem_count.mutate(1);
                *int_ptr = ywThreadPool::get_thread_id();
            }
        }
        /*
        while ((freeable = ywRcuRef::get_freeable_item())) {
            rem_count.mutate(1);
            int_ptr = reinterpret_cast<volatile int32_t*>(freeable);
            if (*int_ptr != magic) {
                printf("idx : %d\n", ywRcuRef::get_slot_idx(&ptr));
                printf("slot: %lld\n", ywRcuRef::get_slot(&ptr));
                printf("fix : %lld\n", ywRcuRef::get_fix_slot(&ptr));
                assert(false);
            }
            *int_ptr = ywThreadPool::get_thread_id();
        }
        */
    }
}

void rcu_ref_test() {
    ywThreadPool  *tpool = ywThreadPool::get_instance();
    int            processor_count = ywThreadPool::get_processor_count();
    int            i;
    int            init = magic;
    int            test = magic;

    printf("idx : %d\n", ywRcuRef::get_slot_idx(&ptr));
    printf("slot: %lld\n", ywRcuRef::get_slot(&ptr));

    ywRcuRef::lock(&ptr);
    printf("idx : %d\n", ywRcuRef::get_slot_idx(&ptr));
    printf("slot: %lld\n", ywRcuRef::get_slot(&ptr));
    ywRcuRef::release(&ptr);

    printf("idx : %d\n", ywRcuRef::get_slot_idx(&ptr));
    printf("slot: %lld\n", ywRcuRef::get_slot(&ptr));

    ptr = &init;
    ywRcuRef::set(&ptr, &test);
    printf("idx : %d\n", ywRcuRef::get_slot_idx(&ptr));
    printf("slot: %lld\n", ywRcuRef::get_slot(&ptr));

//    processor_count = 1;
    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(rcu_ref_task,
                    reinterpret_cast<void*>(i)));
    }
    tpool->wait_to_idle();
    printf("rem_count  %lld\n", rem_count.get());
    printf("free_count %d\n", ywRcuRef::get_free_count());
}


