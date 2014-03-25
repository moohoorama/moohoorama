/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrcuref.h>
#include <gtest/gtest.h>

ywMemPool<ywrcu_free_queue>  ywRcuRef::rc_free_pool;


class ywRcuTestClass {
 public:
    static const int32_t YWR_TEST_COUNT = 1024*128;
    static const int32_t YWR_SLOT_COUNT = 1;
    static const int32_t YWR_MAGIC = 0x123456;

    ywRcuTestClass():reuse_cnt(0), wait_cnt(0) {
    }
    int32_t  **ptr;
    ywRcuRef  *rcu;
    int32_t    operation;
    int32_t    reuse_cnt;
    int32_t    wait_cnt;
    int32_t    slot[YWR_SLOT_COUNT];

    static int estimated_reuse_cnt() {
        return YWR_TEST_COUNT - YWR_SLOT_COUNT;
    }

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
    ywRcuRef        rcu;
    int32_t         init = 0x123456;
    int32_t        *test_ptr = &init;

    /* don't care dup unfix*/
    rcu.unfix();
    rcu.unfix();
    rcu.unfix();
    rcu.unfix();
    rcu.fix();
    rcu.unfix();

    rcu.fix();                              /*fix1*/
    rcu.lock();                             /*lock1*/
    rcu.regist_free_obj(test_ptr);
    rcu.release();

    EXPECT_FALSE(rcu.get_reusable_item());  /*fail by fix1 */

    rcu.lock();                             /*lock2*/
    rcu.release();

    EXPECT_FALSE(rcu.get_reusable_item());  /*fail by fix1*/

    rcu.fix();                              /*fix2*/
    EXPECT_FALSE(rcu.get_reusable_item());  /*fail by fix1,2*/
    rcu.unfix();

    EXPECT_FALSE(rcu.get_reusable_item());  /*fail by fix1,2*/

    rcu.lock();                             /* lock3 */
    rcu.unfix();
    rcu.update_reusable_time();
    EXPECT_TRUE(rcu.get_reusable_item());   /*success don't care lock3*/
    rcu.release();
}

void auto_fix_test() {
    ywRcuRef        rcu;
    int32_t         init = 0x123456;
    int32_t        *test_ptr = &init;
    ywRcuGuard      guard(&rcu);

    rcu.lock();                             /*lock1*/
    rcu.regist_free_obj(test_ptr);
    rcu.release();

    EXPECT_FALSE(rcu.get_reusable_item());
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
    auto_fix_test();

    /*CuncurrencyTest*/
    test_ptr = &init;
    processor_count = 4;
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
    EXPECT_EQ(ywRcuTestClass::estimated_reuse_cnt()*processor_count, val);

    val = 0;
    for (i = 0; i < processor_count; ++i) {
        val += tc[i].wait_cnt;
    }
    printf("wait_cnt : %d\n", val);
    EXPECT_GT(val, 0);
}


