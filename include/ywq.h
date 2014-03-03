/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWQ_H_
#define INCLUDE_YWQ_H_

#include <ywcommon.h>
#include <ywspinlock.h>

template<typename DATA>
class ywQueue {
 public:
    ywQueue():prev(this), next(this), count(0) {
    }

    void init() {
        prev = this;
        next = this;
        count = 0;
    }

    void push(ywQueue *node) {
        ywQueue * old_prev;

        do {
            __sync_synchronize();
            old_prev = prev;

            node->prev = old_prev;
            node->next = this;
        } while (!__sync_bool_compare_and_swap(&prev, old_prev, node));

        old_prev->next = node;
    }

    ywQueue  * pop() {
        ywQueue * old_next;
        ywQueue * new_next;

        do {
            __sync_synchronize();
            old_next = next;
            if (old_next == this) {
                return NULL;
            }
            new_next = old_next->next;
            if (new_next->prev != old_next) {
                continue;
            }
        } while (!__sync_bool_compare_and_swap(&next, old_next, new_next));

        return old_next;
    }

    /*
    void push(ywQueue *node) {
        while (!lock.WLock()) {
        }
        node->next = this;
        node->prev = prev;
        prev->next = node;
        prev = node;
        count++;
        lock.release();
    }
    ywQueue  * pop() {
        ywQueue * ret = NULL;
        while (!lock.WLock()) {
        }
        if (next != this) {
            ret = next;
            ret->next->prev = this;
            next = ret->next;
        }
        count--;
        lock.release();

        return ret;
    }
    */

    ywSpinLock   lock;
    ywQueue    * prev;
    ywQueue    * next;
    DATA         data;
    int32_t      count;
};

void     ywq_test();

#endif  // INCLUDE_YWQ_H_
