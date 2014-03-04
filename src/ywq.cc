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

#define YWQ_PUSH0(x) push[x] = head.push_before(&slot[x])
#define YWQ_PUSH_E(x) do {                          \
        if (push[x] && !head.is_retry(push[x])) {   \
            head.push_after(push[x], &slot[x]);     \
        }                                           \
    } while (0)
#define YWQ_POP0(x)  popr[x] = head.pop_before(&pop1[x], &pop2[x])
#define YWQ_POP__E(x) do {                          \
        if (popr[x] && !head.is_retry(popr[x])) {   \
            head.pop_after(pop1[x], pop2[x]);       \
        }                                           \
    } while (0)

#define YWQ_PUSH_P(x)  assert(!head.is_retry(YWQ_PUSH0(x)))
#define YWQ_PUSH_F(x)  assert(head.is_retry(YWQ_PUSH0(x)))
#define YWQ_POP__P(x)   assert(!head.is_retry(YWQ_POP0(x)))
#define YWQ_POP__F(x)   assert(head.is_retry(YWQ_POP0(x)))

#define YWQ_END()   do {                         \
        printf("[%d] cnt:%d [%d, %d, %d, %d]\n", \
               k++,                              \
               head.get_count(),                 \
               !!popr[0],                        \
               !!popr[1],                        \
               !!popr[2],                        \
               !!popr[3]);                       \
        init();                                  \
    } while (0);

class ywqTestClass {
    ywQueueHead<int32_t> head;
    ywQueue<int32_t>     slot[4];
    ywQueue<int32_t>    *push[4];
    ywQueue<int32_t>    *pop1[4], *pop2[4], *popr[4];
    int32_t              k;

 public:
    ywqTestClass(): k(0) {
    }
    void init() {
        int i;
        head.init();
        for (i = 0; i < 4; ++i) {
            push[i] = 0;
            pop1[i] = 0;
            pop2[i] = 0;
            popr[i] = 0;
        }
    }
    void test();
    void test_all();

 private:
    void try_op(int idx);
};

void ywqTestClass::try_op(int idx) {
    int op     = idx / 4;
    int thread = idx % 4;

    printf("%d : %d\n", op, thread);
    switch (op) {
        case 0:
            YWQ_PUSH0(thread);
            break;
        case 1:
            YWQ_PUSH_E(thread);
            break;
        case 2:
            YWQ_POP0(thread);
            break;
        case 3:
            YWQ_POP__E(thread);
            break;
    }
}

void ywqTestClass::test_all() {
    int i;
    int j;
    int val;
    int thread_count = 3;
    int op_set = 4 * thread_count;
    int op_count = 4;
    int max;

    max = 1;
    for (j = 0; j < op_count; ++j) {
        max *= op_set;
    }
    printf("MAX:%d\n", max);

    for (i = 0; i < max; i++) {
        init();
        val = i;
        for (j = 0; j < op_count; ++j) {
            try_op(val % op_set);
            val /=op_set;
        }
        YWQ_END();
    }
}

void ywqTestClass::test() {
    init();
    YWQ_POP__P(0);
    YWQ_END();

    init();
    YWQ_PUSH_E(0);
    YWQ_PUSH_E(0);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
    YWQ_PUSH_E(0);
    YWQ_POP__P(0);
    YWQ_POP__E(0);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
    YWQ_POP__P(0);
    YWQ_PUSH_E(0);
    YWQ_POP__E(0);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
    YWQ_POP__P(0);
    YWQ_PUSH_E(0);
    YWQ_POP__P(0);
    YWQ_POP__E(0);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
    YWQ_POP__P(0);
    YWQ_POP__E(0);
    YWQ_PUSH_E(0);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
                    YWQ_PUSH_P(1);
    YWQ_PUSH_E(0);
                    YWQ_PUSH_E(1);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
                    YWQ_PUSH_P(1);
                    YWQ_PUSH_E(1);
    YWQ_PUSH_E(0);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
    YWQ_PUSH_E(0);
    YWQ_POP__P(0);
                    YWQ_PUSH_F(1);
                    YWQ_PUSH_F(1);
    YWQ_POP__E(0);
                    YWQ_PUSH_P(1);
                    YWQ_PUSH_E(1);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
    YWQ_PUSH_E(0);
    YWQ_POP__P(0);
                    YWQ_POP__F(1);
    YWQ_POP__E(0);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
                    YWQ_PUSH_P(1);
                    YWQ_PUSH_E(1);
                                    YWQ_POP__P(2);
    YWQ_PUSH_E(0);
                                    YWQ_POP__E(2);
    YWQ_END();

    init();
    YWQ_PUSH_P(0);
                    YWQ_PUSH_P(1);
    YWQ_PUSH_E(0);
                                    YWQ_POP__F(2);
                    YWQ_PUSH_E(1);
                                    YWQ_POP__E(2);
    YWQ_END();
}


void   ywq_test() {
    int          i;
    int          j;
    ywqTestClass tc;

    ywq_test_basic();
    tc.test();
    tc.test_all();

    printf("Push 1    Pop 1\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        for (j = 0; j < THREAD_COUNT; j ++) {
            ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                    pushRoutine, reinterpret_cast<void*>(j)));
        }
        ywThreadPool::get_instance()->wait_to_idle();

        assert(head.calc_count() == TRY_COUNT*THREAD_COUNT);

        for (j = 0; j < THREAD_COUNT; j ++) {
            ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                    popRoutine, reinterpret_cast<void*>(j)));
        }
        ywThreadPool::get_instance()->wait_to_idle();

        assert(head.calc_count() == 0);
    }

    printf("Push 1  &  Pop 1\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        assert(head.calc_count() == 0);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                popRoutine, reinterpret_cast<void*>(1)));
        usleep(10000);
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                pushRoutine, reinterpret_cast<void*>(0)));
        ywThreadPool::get_instance()->wait_to_idle();

        assert(head.calc_count() == 0);
    }

    printf("Push 2  &  Pop 1\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        assert(head.calc_count() == 0);
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
        assert(head.calc_count() == 0);
    }

    printf("Push 1  &  Pop 2\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        assert(head.calc_count() == 0);
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

        assert(head.calc_count() == 0);
    }

    printf("Push 2  &  Pop 2\n");
    for (i = 0; i < ITER_COUNT; i ++) {
        head.init();
        assert(head.calc_count() == 0);
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

        assert(head.calc_count() == 0);
    }
}
