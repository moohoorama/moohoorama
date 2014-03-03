/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWQ_H_
#define INCLUDE_YWQ_H_

#include <ywcommon.h>
#include <ywspinlock.h>

template<typename DATA>
class ywQueue {
 public:
    ywQueue():prev(this), next(this) {
    }

    void init() {
        prev = this;
        next = this;
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

    ywQueue    * prev;
    ywQueue    * next;
    DATA         data;
};

void     ywq_test();

#endif  // INCLUDE_YWQ_H_
