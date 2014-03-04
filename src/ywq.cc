/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gtest/gtest.h>

#include <ywthread.h>
#include <ywq.h>

const int32_t THREAD_COUNT = 4;
const int32_t ITER_COUNT = 8;
const int32_t TRY_COUNT  = 1024*64;

static ywQueueHead<int32_t> head;
static ywQueue<int32_t> slot[THREAD_COUNT][TRY_COUNT];

void pushRoutine(void * arg) {
    int          num = reinterpret_cast<int>(arg);
    int          i;

    for (i = 0; i < TRY_COUNT; i ++) {
        slot[num][i].data = i + 100000;
        head.push(&slot[num][i]);
    }
}
void popRoutine(void * arg) {
    ywQueue<int32_t> * iter;
    int          i;
    int          prev      = 0;

    prev = 100000;
    for (i = 0; i < TRY_COUNT; i ++) {
        while (!(iter = head.pop())) {
        }
        assert(iter != head.getEnd());
        prev++;
    }
}

void   ywq_test() {
    int          i;
    int          j;

    head.init();
    (void) head.pop();
    slot[0][0].data = 10000;
    head.push(&slot[0][0]);
    (void) head.pop();
    head.push(&slot[0][0]);
    (void) head.pop();

    printf("Push 1    Pop 1\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        for (j = 0; j < THREAD_COUNT; j ++) {
            ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                    pushRoutine, reinterpret_cast<void*>(j)));
        }
        ywThreadPool::get_instance()->wait_to_idle();

        assert(head.get_count() == TRY_COUNT*THREAD_COUNT);

        for (j = 0; j < THREAD_COUNT; j ++) {
            ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                    popRoutine, reinterpret_cast<void*>(j)));
        }
        ywThreadPool::get_instance()->wait_to_idle();

        assert(head.get_count() == 0);
    }

    printf("Push 1  &  Pop 1\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        assert(head.get_count() == 0);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        usleep(10000);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(0)));
        ywThreadPool::get_instance()->wait_to_idle();

        assert(head.get_count() == 0);
    }

    printf("Push 2  &  Pop 1\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        assert(head.get_count() == 0);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        usleep(10000);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(0)));
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(1)));
        ywThreadPool::get_instance()->wait_to_idle();

        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        ywThreadPool::get_instance()->wait_to_idle();
        assert(head.get_count() == 0);
    }

    printf("Push 1  &  Pop 2\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        assert(head.get_count() == 0);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(0)));
        ywThreadPool::get_instance()->wait_to_idle();
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        usleep(10000);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(1)));
        ywThreadPool::get_instance()->wait_to_idle();

        assert(head.get_count() == 0);
    }

    printf("Push 2  &  Pop 2\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        assert(head.get_count() == 0);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        usleep(10000);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(0)));
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(1)));
        ywThreadPool::get_instance()->wait_to_idle();

        assert(head.get_count() == 0);
    }
}
