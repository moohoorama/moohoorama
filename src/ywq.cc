/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gtest/gtest.h>

#include <ywthread.h>
#include <ywq.h>

#define YWQ_FOREACH(i, h)  for ((i) = (h)->next; (i) != (h); (i) = (i)->next)

void printList();

const int32_t ITER_COUNT = 8;
const int32_t TRY_COUNT  = 1024*64;

static ywQueue<int32_t> head;
static ywQueue<int32_t> slot[16][TRY_COUNT];

void printList() {
    ywQueue<int32_t> * iter;

    YWQ_FOREACH(iter, &head) {
        printf("[%8x %8x %8d %8x]\n",
                (unsigned int)iter->prev,
                (unsigned int)iter, iter->data,
                (unsigned int)iter->next);
    }
    printf("\n");
}

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
    int          prevRatio = 0;
    int          tryCount  = 0;

    prev = 100000;
    for (i = 0; i < tryCount; i ++) {
        while (!(iter = head.pop())) {
        }
        if (static_cast<int>(iter->data) != prev) {
            assert(static_cast<int>(iter->data) == prev);
        }
        if (i*10/TRY_COUNT != prevRatio) {
            printf("%d%%\n", prevRatio);
            prevRatio = i*10/TRY_COUNT;
        }
        prev++;
    }
}

#define THREAD_COUNT 16
void   ywq_test() {
    int          i;
    int          j;
    int          k;
    ywQueue<int32_t> * iter;

    slot[0][0].data = 10000;
    head.push(&slot[0][0]);
    iter = head.pop();
    head.push(&slot[0][0]);
    iter = head.pop();

    for (i = 0; i < ITER_COUNT; i ++) {
        for (j = 0; j < THREAD_COUNT; j ++) {
            ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                    pushRoutine, reinterpret_cast<void*>(j)));
        }
        ywThreadPool::get_instance()->wait_to_idle();

        k = 0;
        YWQ_FOREACH(iter, &head)
            k++;
        assert(k == TRY_COUNT*THREAD_COUNT);
    }

    new (&head) ywQueue<int32_t>;
    for (i = 0; i < ITER_COUNT; i ++) {
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        usleep(10000);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(0)));
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(0)));
        ywThreadPool::get_instance()->wait_to_idle();
    }
}
