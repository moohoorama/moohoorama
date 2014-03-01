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

    /*
    void push(ywQueue *node) {
        ywQueue * old_prev;

        do {
            old_prev = prev;
            __sync_synchronize();

            node->prev = old_prev;
            node->next = this;
        } while (!__sync_bool_compare_and_swap(&prev, old_prev, node));
        __sync_synchronize();

        old_prev->next = node;
    }

    ywQueue  * pop() {
        ywQueue * target = NULL;
        ywQueue * linker = NULL;

        do {
            __sync_synchronize();
            if (prev == this) {
                return NULL;  // Really Emtpy
            }

            target = next;
            linker = target->next;
            if (target->prev != this) {
                continue;
            }
        } while (!__sync_bool_compare_and_swap(&linker->prev, target, this));

        next = linker;

        return target;
    }
    */

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

    ywSpinLock   lock;
    ywQueue    * prev;
    ywQueue    * next;
    DATA         data;
    int32_t      count;
};

void     ywq_test();

#endif  // INCLUDE_YWQ_H_
