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
            slot[i].data = 0;
        }
    }
    void end() {
#ifdef REPORT
        printf("[%d] [%d, %d, %d, %d]\n",
               k++,
               !!popr[0],
               !!popr[1],
               !!popr[2],
               !!popr[3]);
#endif
    }
    void test();
    void test_all();

    ywQueue<int32_t> *push_begin(int x) {
        if (!slot[x].data) {
            push[x] = head.push_before(&slot[x]);
            if (push[x] && !head.is_retry(push[x])) {
                slot[x].data = 1;
            }
            return push[x];
        }
        return push[x];
    }

    void push_begin(int x, bool ret) {
        assert((!head.is_retry(push_begin(x))) == ret);
    }

    void push_end(int x) {
        if (push[x] && !head.is_retry(push[x])) {
            head.push_after(push[x], &slot[x]);
            push[x] = NULL;
        }
    }

    ywQueue<int32_t> *pop_begin(int x) {
        popr[x] = head.pop_before(&pop1[x], &pop2[x]);
        return  popr[x];
    }
    void pop_begin(int x, bool ret) {
        pop_begin(x);
        if (ret) {
            assert(!head.is_retry(popr[x]) && popr);
        } else {
            assert(head.is_retry(popr[x]));
        }
    }
    void pop_end(int x) {
        if (popr[x] && !head.is_retry(popr[x])) {
            head.pop_after(pop1[x], pop2[x]);
            popr[x] = NULL;
        }
    }

 private:
    void try_op(int idx);
};

void ywqTestClass::try_op(int idx) {
//    const char op_name[][8]={"pushb", "pushe", "popb", "pope"};
    int op     = idx / 4;
    int thread = idx % 4;

    //  printf("%8s : %d\n", op_name[op], thread);
    switch (op) {
        case 0:
            push_begin(thread);
            break;
        case 1:
            push_end(thread);
            break;
        case 2:
            pop_begin(thread);
            break;
        case 3:
            pop_end(thread);
            break;
    }
}

void ywqTestClass::test_all() {
    int i;
    int j;
    int val;
    int thread_count = 3;
    int op_set = 4 * thread_count;
    int op_count = 6;
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
        end();
    }
}

void ywqTestClass::test() {
    init();
    pop_begin(0, true);
    end();

    init();
    push_end(0);
    push_end(0);
    end();

    init();
    push_begin(0, true);
    push_end(0);
    pop_begin(0, true);
    pop_end(0);
    end();

    init();
    push_begin(0, true);
    pop_begin(0, true);
    push_end(0);
    pop_end(0);
    end();

    init();
    push_begin(0, true);
    pop_begin(0, true);
    push_end(0);
    pop_begin(0, true);
    pop_end(0);
    end();

    init();
    push_begin(0, true);
    pop_begin(0, true);
    pop_end(0);
    push_end(0);
    end();

    init();
    push_begin(0, true);
                    push_begin(1, true);
    push_end(0);
                    push_end(1);
    end();

    init();
    push_begin(0, true);
                    push_begin(1, true);
                    push_end(1);
    push_end(0);
    end();

    init();
    push_begin(0, true);
    push_end(0);
    pop_begin(0, true);
                    push_begin(1, false);
                    push_begin(1, false);
    pop_end(0);
                    push_begin(1, true);
                    push_end(1);
    end();

    init();
    push_begin(0, true);
    push_end(0);
    pop_begin(0, true);
                    pop_begin(1, false);
    pop_end(0);
    end();

    init();
    push_begin(0, true);
                    push_begin(1, true);
                    push_end(1);
                                    pop_begin(2);
    push_end(0);
                                    pop_end(2);
    end();

    init();
    push_begin(0, true);
                    push_begin(1, true);
    push_end(0);
                                    pop_begin(2, false);
                    push_end(1);
                                    pop_end(2);
    end();

    init();
    push_begin(0, true);
    push_end(0);
                    push_begin(0, true);
                    push_end(1);
    pop_begin(0, true);
                    pop_begin(1, false);
    end();
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
