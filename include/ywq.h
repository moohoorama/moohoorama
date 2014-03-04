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
        use_dummy = false;
    }

    void push(ywQueue<DATA> *node) {
        ywQueue<DATA> * old_next;

        do {
            __sync_synchronize();
            old_next = head.next;

            node->next = old_next;
            node->prev = &head;
            assert(node->next != node);
            assert(node->prev != node);
        } while (!__sync_bool_compare_and_swap(&head.next, old_next, node));

        assert(__sync_bool_compare_and_swap(&old_next->prev, &head, node));
    }

    ywQueue<DATA>  * pop() {
        ywQueue<DATA> * old_prev;
        ywQueue<DATA> * new_prev;
        ywQueue<DATA> * ret = NULL;

        while (true) {
            __sync_synchronize();
            old_prev = head.prev;
            new_prev = old_prev->prev;
            if (old_prev == &head) {
                break;
            }
            if (new_prev->next != old_prev) {
                continue;
            }
            if (new_prev == &head) {
                if (old_prev == &dummy) {
                    break;
                }
                if (!dummy_lock.WLock()) {
                    return NULL;
                }
                if (!use_dummy) {
                    push(&dummy);
                    use_dummy = true;
                }
                dummy_lock.release();
                continue;
            }
            if (__sync_bool_compare_and_swap(&head.prev, old_prev, new_prev)) {
                assert(__sync_bool_compare_and_swap(
                        &new_prev->next, old_prev, &head));
                if (old_prev == &dummy) {
                    while (!dummy_lock.WLock()) {
                    }
                    use_dummy = false;
                    dummy_lock.release();
                    continue;
                }
                ret = old_prev;
                break;
            }
        }

        return ret;
    }

    ywQueue<DATA> *getNext() {
        return head.next;
    }
    ywQueue<DATA> *getEnd() {
        return &head;
    }
    ywQueue<DATA> *getDummy() {
        return &dummy;
    }
    size_t get_count() {
        size_t k = 0;
        ywQueue<int32_t> * iter;

        for (iter = head.next; iter != &head; iter = iter->next) {
            if (iter != &dummy) {
                k++;
            }
        }
        return k;
    }

 private:
    ywQueue<DATA> head;
    ywQueue<DATA> dummy;
    ywSpinLock    dummy_lock;
    bool          use_dummy;
};

void     ywq_test();

#endif  // INCLUDE_YWQ_H_
