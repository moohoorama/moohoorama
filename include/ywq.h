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
    ywQueue    * prev;
    ywQueue    * next;
    DATA         data;
};

#define YWQ_FOREACH(i, h)   \
    for ((i) = (h)->getNext(); (i) != (h)->getEnd(); (i) = (i)->next)

template<typename DATA>
class ywQueueHead {
 public:
    ywQueueHead() {
        init();
    }
    void init() {
        head.next = &head;
        head.prev = &head;
    }

    void push(ywQueue<DATA> *node) {
        ywQueue<DATA> * old_next;

        do {
            do {
                __sync_synchronize();
                old_next = head.next;
                node->next = old_next;
                node->prev = &head;
            } while (old_next->prev != &head);
        } while (!__sync_bool_compare_and_swap(&head.next, old_next, node));

        assert(__sync_bool_compare_and_swap(&old_next->prev, &head, node));
    }
    ywQueue<DATA>  * pop() {
        ywQueue<DATA> * prev;
        ywQueue<DATA> * prev_prev;

        do {
            do {
                __sync_synchronize();
                prev = head.prev;
                prev_prev = prev->prev;
                if (prev == &head) {
                    return NULL;
                }
            } while ((prev->next != &head) && (prev_prev->next == prev));
        } while (!__sync_bool_compare_and_swap(&prev_prev->next, prev, &head));

        assert(__sync_bool_compare_and_swap(&head.prev, prev, prev_prev));

        return prev;
    }

    ywQueue<DATA> *getNext() {
        return head.next;
    }
    ywQueue<DATA> *getEnd() {
        return &head;
    }
    size_t get_count() {
        size_t k = 0;
        ywQueue<int32_t> * iter;

        for (iter = head.next; iter != &head; iter = iter->next) {
            k++;
        }
        return k;
    }

 private:
    ywQueue<DATA> head;
};

void     ywq_test();

#endif  // INCLUDE_YWQ_H_
