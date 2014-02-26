/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gtest/gtest.h>

#include <ywthread.h>
#include <ywq.h>

void printList();
void   ywqInit(struct ywq * _head) {
    _head->prev = _head;
    _head->next = _head;
    _head->data = NULL;
    _head->meta = 9999;
    _head->size = 0;
}
void   ywqPush(struct ywq * _head, struct ywq * _target) {
    struct ywq * oldPrev;
    do {
        oldPrev = _head->prev;
        __sync_synchronize();
        _target->prev = oldPrev;
        _target->next = _head;
    } while (!__sync_bool_compare_and_swap(&_head->prev, oldPrev, _target));

    __sync_synchronize();
    __sync_add_and_fetch(&_head->size, 1);

    oldPrev->next = _target;
}
struct ywq  * ywqPop(struct ywq * _head) {
    struct ywq * target = NULL;
    struct ywq * linker = NULL;

    if (_head->prev == _head) {
        __sync_synchronize();
        return NULL; /* Really Emtpy */
    }

    do {
        target = _head->next;
        linker = target->next;
        __sync_synchronize();
        while (target->prev != _head) {
            linker = target;
            target = target->prev;
        }
        __sync_synchronize();
    } while (!__sync_bool_compare_and_swap(&linker->prev, target, _head));

    _head->next = linker;

    return target;
}

#define ITER_COUNT 8
#define TRY_COUNT  1024
static struct ywq head;
static struct ywq slot[16][TRY_COUNT];

void printList() {
    struct ywq * iter;

    YWQ_FOREACH(iter, &head) {
        printf("[%8x %8x %8d %8x]\n",
                (unsigned int)iter->prev,
                (unsigned int)iter, iter->meta,
                (unsigned int)iter->next);
    }
    printf("\n");
}

void pushRoutine(void * arg) {
    int          num = reinterpret_cast<int>(arg);
    int          i;

    for (i = 0; i < TRY_COUNT; i ++) {
        slot[num][i].meta = i + 100000;
        ywqPush(&head, &slot[num][i]);
    }
}
void popRoutine(void * arg) {
    struct ywq * iter;
    int          i;
    int          prev      = 0;
    int          prevRatio = 0;
    int          tryCount  = 0;

    prev = 100000;
    for (i = 0; i < tryCount; i ++) {
        while (!(iter = ywqPop(&head))) {
        }
        if (static_cast<int>(iter->meta) != prev) {
            assert(static_cast<int>(iter->meta) == prev);
        }
        if (i*10/TRY_COUNT != prevRatio) {
            printf("%d%%\n", prevRatio);
            prevRatio = i*10/TRY_COUNT;
        }
        prev++;
    }
}

#define THREAD_COUNT 16
void   ywqTest() {
    int          i;
    int          j;
    int          k;
    struct ywq * iter;


    ywqInit(&head);
    slot[0][0].meta = 10000;
    ywqPush(&head, &slot[0][0]);
    iter = ywqPop(&head);
    ywqPush(&head, &slot[0][0]);
    iter = ywqPop(&head);

    for (i = 0; i < ITER_COUNT; i ++) {
        ywqInit(&head);

        for (j = 0; j < THREAD_COUNT; j ++) {
            ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                    pushRoutine, reinterpret_cast<void*>(j)));
        }
        ywThreadPool::get_instance()->wait_to_idle();

        k = 0;
        YWQ_FOREACH_PREV(iter, &head)
            k++;
        assert(k == TRY_COUNT*THREAD_COUNT);
    }

    ywqInit(&head);
    for (i = 0; i < ITER_COUNT; i ++) {
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        usleep(10000);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(0)));
        ywThreadPool::get_instance()->wait_to_idle();
    }
}
