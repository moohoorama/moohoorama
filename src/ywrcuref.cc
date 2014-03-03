/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrcuref.h>
#include <gtest/gtest.h>
#include <ywsll.h>

ywMemPool<ywrcu_free_queue>  ywRcuRef::rc_free_pool;


class ywrcuTestClass {
    static const int32_t YWR_TEST_COUNT = 1024*128;
    static const int32_t YWR_SLOT_COUNT = 64;
    static const int32_t YWR_MAGIC = 0x123456;

 public:
    ywrcuTestClass():reuse_cnt(0), wait_cnt(0) {
    }
    int32_t  **ptr;
    ywRcuRef  *rcu;
    int32_t    reuse_cnt;
    int32_t    wait_cnt;
    int32_t    slot[YWR_SLOT_COUNT];

    void run();
};

void rcu_ref_task(void *arg) {
    ywrcuTestClass *tc = reinterpret_cast<ywrcuTestClass*>(arg);

    tc->run();
}

void ywrcuTestClass::run() {
    int32_t   val;
    int32_t  *nval;
    int32_t  *oval;
    int32_t   i;

    for (i = 0; i < YWR_TEST_COUNT; ++i) {
        rcu->fix();
        val = *const_cast<volatile int32_t *>(*ptr);
        if (val != YWR_MAGIC) {
            printf("Error: %d\n", val);
            assert(false);
        }
        rcu->unfix();

        if (i < YWR_SLOT_COUNT) {
            nval = &slot[i];
        } else {
            while (!(nval =
                    reinterpret_cast<int32_t*>(rcu->get_reusable_item()))) {
                wait_cnt++;
            }
            reuse_cnt++;
        }

        while (!rcu->lock()) {
        }
        oval = *ptr;
        rcu->regist_free_obj(oval);
        *oval = rcu->get_global_ref();
        *nval = YWR_MAGIC;
        *ptr = nval;
        rcu->release();
    }
}

void rcu_ref_test() {
    ywThreadPool   *tpool = ywThreadPool::get_instance();
    ywRcuRef        rcu;
    ywrcuTestClass  tc[MAX_THREAD_COUNT];
    int32_t         processor_count = ywThreadPool::get_processor_count();
    int32_t        *test_ptr;
    int32_t         init = 0x123456;
    int32_t         i;
    int32_t         val;

    test_ptr = &init;

//    processor_count = 1;
    for (i = 0; i < processor_count; ++i) {
        tc[i].ptr = &test_ptr;
        tc[i].rcu = &rcu;
        EXPECT_TRUE(tpool->add_task(rcu_ref_task, &tc[i]));
//                    reinterpret_cast<void*>(i)));
    }
    tpool->wait_to_idle();

    val = 0;
    for (i = 0; i < processor_count; ++i) {
        val += tc[i].reuse_cnt;
    }
    printf("reuse_cnt : %d\n", val);

    val = 0;
    for (i = 0; i < processor_count; ++i) {
        val += tc[i].wait_cnt;
    }
    printf("wait_cnt  : %d\n", val);
}


