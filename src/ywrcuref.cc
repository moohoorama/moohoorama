/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrcuref.h>
#include <gtest/gtest.h>
#include <ywsll.h>

ywMemPool<ywrcu_free_queue>  ywRcuRef::rc_free_pool;


class ywRcuTestClass {
    static const int32_t YWR_TEST_COUNT = 1024*128;
    static const int32_t YWR_SLOT_COUNT = 128;
    static const int32_t YWR_MAGIC = 0x123456;

 public:
    ywRcuTestClass():reuse_cnt(0), wait_cnt(0) {
    }
    int32_t  **ptr;
    ywRcuRef  *rcu;
    int32_t    operation;
    int32_t    reuse_cnt;
    int32_t    wait_cnt;
    int32_t    slot[YWR_SLOT_COUNT];

    void run();
};

void rcu_ref_task(void *arg) {
    ywRcuTestClass *tc = reinterpret_cast<ywRcuTestClass*>(arg);

    tc->run();
}

void ywRcuTestClass::run() {
    int32_t   val;
    int32_t  *nval;
    int32_t  *oval;
    int32_t   i;

    switch (operation) {
        case 0:
            assert(!rcu->get_reusable_item());
            return;
        case 1:
            assert(rcu->get_reusable_item());
            return;
    }

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
        *nval = YWR_MAGIC;
        *ptr = nval;
//      *oval = rcu->get_global_ref();
        rcu->release();
    }
}

void basic_test() {
    ywThreadPool   *tpool = ywThreadPool::get_instance();
    ywRcuRef        rcu;
    ywRcuTestClass  tc;
    int32_t        *test_ptr;
    int32_t         init = 0x123456;

    /*BasicTest*/
    test_ptr  = &init;
    tc.ptr    = &test_ptr;
    tc.rcu    = &rcu;

    rcu.fix();
    rcu.lock();
    rcu.regist_free_obj(test_ptr);
    rcu.release();

    tc.operation = 0; /* should fail reusing */
    EXPECT_TRUE(tpool->add_task(rcu_ref_task, &tc));
    tpool->wait_to_idle();

    rcu.lock();
    rcu.release();

    tc.operation = 0; /* should fail reusing */
    EXPECT_TRUE(tpool->add_task(rcu_ref_task, &tc));
    tpool->wait_to_idle();
    rcu.unfix();

    tc.operation = 1; /* success */
    EXPECT_TRUE(tpool->add_task(rcu_ref_task, &tc));
    tpool->wait_to_idle();

    rcu.lock();
    rcu.fix();
    rcu.regist_free_obj(test_ptr);
    rcu.release();

    tc.operation = 0; /* should fail reusing */
    EXPECT_TRUE(tpool->add_task(rcu_ref_task, &tc));
    tpool->wait_to_idle();

    rcu.lock();
    rcu.release();

    tc.operation = 0; /* should fail reusing */
    EXPECT_TRUE(tpool->add_task(rcu_ref_task, &tc));
    tpool->wait_to_idle();
    rcu.unfix();

    tc.operation = 1; /* success */
    EXPECT_TRUE(tpool->add_task(rcu_ref_task, &tc));
    tpool->wait_to_idle();
}

void rcu_ref_test() {
    ywThreadPool   *tpool = ywThreadPool::get_instance();
    ywRcuRef        rcu;
    ywRcuTestClass  tc[MAX_THREAD_COUNT];
    int32_t         processor_count = ywThreadPool::get_processor_count();
    int32_t        *test_ptr;
    int32_t         init = 0x123456;
    int32_t         i;
    int32_t         val;

    basic_test();

    /*CuncurrencyTest*/
    test_ptr = &init;
    processor_count = 2;
    for (i = 0; i < processor_count; ++i) {
        tc[i].operation = 2;
        tc[i].ptr       = &test_ptr;
        tc[i].rcu       = &rcu;
        EXPECT_TRUE(tpool->add_task(rcu_ref_task, &tc[i]));
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


