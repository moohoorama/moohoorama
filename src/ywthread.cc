/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywthread.h>
#include <gtest/gtest.h>

ywThreadPool ywThreadPool::gInstance;

void ywThreadPool::init() {
    pthread_attr_t  attr;
    int32_t         i;
    cpu_set_t      *cmask;

    assert(cmask = CPU_ALLOC(thread_count));
    assert(0 == pthread_attr_init(&attr) );

    for (i = 0; i < thread_count; ++i) {
        assert(0 == pthread_create(
                &pt[i], &attr, ywThreadPool::work, NULL));
        CPU_ZERO_S(thread_count, cmask);
        CPU_SET_S(i, (size_t)thread_count, cmask);
        assert(0 == pthread_setaffinity_np(pt[i], thread_count, cmask));
    }
    block_add_task = false;
}


void *ywThreadPool::work(void * /*arg_ptr*/) {
    ywThreadPool *tpool = ywThreadPool::get_instance();
    ywTaskFunc    func;
    void         *arg;
    ywTID         tid = tpool->get_thread_id();

    tpool->running[tid] = true;

    while (!tpool->done) {
        if (tpool->get_task(&func, &arg)) {
            func(arg);
        } else {
            tpool->running[tid] = false;
            usleep(SLEEP_INTERVAL);
            tpool->running[tid] = true;
        }
    }

    return NULL;
}

void sleep_task(void *arg) {
    bool *act = reinterpret_cast<bool*>(arg);

    if (*act == true) {
        usleep(200*1000);
    }
}

void threadpool_test() {
    ywThreadPool *tpool = ywThreadPool::get_instance();
    int           processor_count = ywThreadPool::get_processor_count();
    bool          act = true;
    int           i;

    /* TEST add_task */
    for (i = 0; i < ywThreadPool::MAX_QUEUE_SIZE-1; ++i) {
        EXPECT_TRUE(tpool->add_task(sleep_task, &act));
    }
    usleep(100*1000);

    /* TEST thread running */
    EXPECT_EQ(processor_count, tpool->get_running_thread_count());
    for (i = 0; i < processor_count; ++i) {
        EXPECT_TRUE(tpool->add_task(sleep_task, &act));
    }

    /* TEST queue_limitation */
    EXPECT_FALSE(tpool->add_task(sleep_task, &act));

    /* TEST wait_to_idle */
    act = false;
    tpool->wait_to_idle();
    EXPECT_EQ(0, tpool->get_running_thread_count());

    /* TEST block_add_task */
    tpool->set_block_add_task(true);
    EXPECT_FALSE(tpool->add_task(sleep_task, &act));
    tpool->set_block_add_task(false);
    EXPECT_TRUE(tpool->add_task(sleep_task, &act));
}
