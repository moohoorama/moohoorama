/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrcuref.h>
#include <gtest/gtest.h>

ywRcuRefManager ywRcuRefManager::gInstance;

void ywRcuRefManager::regist(ywRcuRef *rcu) {
    while (!lock.WLock()) {}

    list.attach(&rcu->local_list);
    ++rcu_count;

    lock.release();
}

void ywRcuRefManager::unregist(ywRcuRef *rcu) {
    if (!rcu->local_list.is_unlinked()) {
        while (!lock.WLock()) {}

        assert(rcu_count > 0);
        rcu->local_list.detach();
        --rcu_count;

        lock.release();
    }
}

void ywRcuRefManager::update(void *arg) {
    gInstance._update();
}
void ywRcuRefManager::_update() {
    ywDList  *iter = list.next;
    ywRcuRef *rcu;

    while (!lock.WLock()) {}

    while (iter != &list) {
        rcu = reinterpret_cast<ywRcuRef*>(
            reinterpret_cast<char*>(iter) - offsetof(ywRcuRef, local_list));
        rcu->update_reusable_time();
        iter = iter->next;
    }
    lock.release();
}


class testType {
 public:
    explicit testType() {
    }
    explicit testType(int64_t _value):value(_value) {
    }

    ywDList rcu_node;
    int64_t value;
};

class ywRcuTestClass {
 public:
    static const int32_t YWR_TEST_COUNT = 1024*128;
    static const int32_t YWR_SLOT_COUNT = 16;
    static const int32_t YWR_MAGIC = 0x123456;

    ywRcuTestClass():reuse_cnt(0), wait_cnt(0) {
    }
    ywRcuRef  *rcu;
    int32_t    operation;
    int32_t    reuse_cnt;
    int32_t    wait_cnt;
    testType **ptr;
    testType   slot[YWR_SLOT_COUNT];

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
    volatile testType *val;
    testType *nval;
    testType *oval;
    int32_t   i;

    switch (operation) {
        case 0:
            nval = rcu->get_reusable_item<testType>();
            assert(!nval);
            return;
        case 1:
            nval = rcu->get_reusable_item<testType>();
            assert(nval);
            return;
    }

    for (i = 0; i < YWR_TEST_COUNT; ++i) {
        rcu->fix();
        val = const_cast<volatile testType *>(*ptr);
        if (val->value != YWR_MAGIC) {
            printf("Error: %"PRId64"\n", val->value);
            assert(false);
        }
        rcu->unfix();

        if (i < YWR_SLOT_COUNT) {
            nval = &slot[i];
        } else {
            while (!(nval = rcu->get_reusable_item<testType>())) {
                wait_cnt++;
            }
            reuse_cnt++;
        }

        while (!rcu->lock()) {
        }
        oval = *ptr;
        rcu->regist_free_obj(oval);
        nval->value = YWR_MAGIC;
        *ptr = nval;
//      *oval = rcu->get_global_ref();
        rcu->release();
    }
    rcu->aging();
}

void basic_test() {
    ywRcuRef        rcu;
    testType        init(0x123456);
    testType       *test_ptr = &init;

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

    EXPECT_FALSE(rcu.get_reusable_item<testType>());  /*fail by fix1 */

    rcu.lock();                                      /*lock2*/
    rcu.release();

    EXPECT_FALSE(rcu.get_reusable_item<testType>());  /*fail by fix1*/

    rcu.fix();                                       /*fix2*/
    EXPECT_FALSE(rcu.get_reusable_item<testType>());  /*fail by fix1,2*/
    rcu.unfix();

    EXPECT_FALSE(rcu.get_reusable_item<testType>());  /*fail by fix1,2*/
    rcu.unfix();

    rcu.aging();
    rcu.update_reusable_time();
    rcu.lock();                                    /*lock3*/
    assert(rcu.get_reusable_item<testType>()); /*success don't care lock3*/
    rcu.release();
}

void auto_fix_test() {
    ywRcuRef        rcu;
    testType        init(0x123456);
    testType       *test_ptr = &init;
    ywRcuGuard      guard(&rcu);

    rcu.lock();                             /*lock1*/
    rcu.regist_free_obj(test_ptr);
    rcu.release();

    EXPECT_FALSE(rcu.get_reusable_item<testType>());
}

void rcu_ref_test() {
    ywWorkerPool   *tpool = ywWorkerPool::get_instance();
    ywRcuRef        rcu;
    ywRcuTestClass  tc[MAX_THREAD_COUNT];
    int32_t         processor_count = ywWorkerPool::get_processor_count();
    testType       *test_ptr;
    testType        init(0x123456);
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


