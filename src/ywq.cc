/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gtest/gtest.h>

#include <ywthread.h>
#include <ywq.h>

class ywqTestClass {
 public:
    static const int32_t TRY_COUNT  = 1024*16;

    ywqTestClass() {
    }

    void set(ywQueueHead<int32_t> *_head, int32_t _op) {
        head = _head;
        op   = _op;
    }

    void pushTest();
    void popTest();
    static void run(void * arg);

 private:
    int32_t               op;
    ywQueueHead<int32_t> *head;
    ywQueue<int32_t>      slot[TRY_COUNT];
};


void ywqTestClass::run(void * arg) {
    ywqTestClass * qtc = reinterpret_cast<ywqTestClass*>(arg);

    switch (qtc->op) {
        case 0:
            qtc->pushTest();
            break;
        case 1:
            qtc->popTest();
            break;
        default:
            assert(false);
    }
}

void ywqTestClass::pushTest() {
    int               i;

    for (i = 0; i < TRY_COUNT; i ++) {
        head->push(&slot[i]);
    }
}

void ywqTestClass::popTest() {
    ywQueue<int32_t> * iter;
    int                i;

    for (i = 0; i < TRY_COUNT; i ++) {
        while (!(iter = head->pop())) {
        }
        assert(iter != head->get_head());
    }
}

void ywq_test_basic() {
    ywQueueHead<int32_t> head;
    ywQueue<int32_t> slot;

    head.init();
    (void) head.pop();
    head.push(&slot);
    (void) head.pop();
    head.push(&slot);
    (void) head.pop();
}
void ywq_move_test() {
    ywQueueHead<int32_t, false>  q1;
    ywQueueHead<int32_t, false>  q2;
    ywQueue<int32_t>             slot[16];
    ywQueue<int32_t>            *iter;
    int32_t                      i;

    q1.init();
    q2.init();

    ASSERT_EQ(0, q1.calc_count());
    ASSERT_EQ(0, q2.calc_count());

    for (i = 0; i < 8; i ++) {
        ASSERT_EQ(i, q1.calc_count());
        ASSERT_EQ(i, q2.calc_count());
        q1.push(&slot[i]);
        q2.push(&slot[i+8]);
        ASSERT_EQ(i+1, q1.calc_count());
        ASSERT_EQ(i+1, q2.calc_count());
    }

    ASSERT_EQ(8, q1.calc_count());
    ASSERT_EQ(8, q2.calc_count());

    q2.bring_all(&q1);

    ASSERT_EQ(0,  q1.calc_count());
    ASSERT_EQ(16, q2.calc_count());

    for (i = 0; i < 8; i ++) {
        iter = q2.get_head()->next->next;

        ASSERT_EQ(i*2,  q1.calc_count());
        ASSERT_EQ(16 - i*2, q2.calc_count());
        q1.bring(q2.get_head(), iter);
        ASSERT_EQ(2 + i*2,  q1.calc_count());
        ASSERT_EQ(14 - i*2, q2.calc_count());
    }
}

void   ywq_test() {
    const int32_t        THREAD_COUNT = 4;
    const int32_t        ITER_COUNT   = 64;
    int                  phase;
    int                  push_task_cnt;
    int                  pop_task_cnt;
    int                  i;
    int                  j;
    ywQueueHead<int32_t> head;
    ywqTestClass         tc[THREAD_COUNT];

    ywq_test_basic();

    ywq_move_test();

    for (phase = 0; phase <= THREAD_COUNT/2; phase++) {
        push_task_cnt = THREAD_COUNT-phase;
        pop_task_cnt  = phase;
        printf("Push : %d    Pop : %d\n", push_task_cnt, pop_task_cnt);
        for (j = 0; j < THREAD_COUNT; j++) {
            tc[j].set(&head, j < phase);
        }

        for (i = 0; i < ITER_COUNT; i ++) {
            head.init();
            for (j = 0; j < THREAD_COUNT; j ++) {
                ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                        ywqTestClass::run, &tc[j]));
            }

            ywThreadPool::get_instance()->wait_to_idle();

            assert(static_cast<int32_t>(head.calc_count()) ==
                   ywqTestClass::TRY_COUNT*(push_task_cnt-pop_task_cnt));

            printf("\r%3d%%", i*100/ITER_COUNT);
            fflush(stdout);
        }
        printf("\r%3d%%\n", i*100/ITER_COUNT);
    }
}
