/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWQ_H_
#define INCLUDE_YWQ_H_

#include <ywcommon.h>
#include <ywspinlock.h>
#include <ywutil.h>

template<typename DATA>
class ywQueue {
 public:
    ywQueue():next(this) {
    }
    ywQueue    * next;
    DATA         data;
};

template<typename DATA>
class ywQueueHead {
 public:
    ywQueueHead():head(&head_instance) {
        init();
    }
    void init() {
        head->next = head;
        tail = head;
    }

    ywQueue<DATA> *get_end() {
        return head;
    }

    void push(ywQueue<DATA> *node) {
        ywQueue<DATA> *cur_tail;

        do {
            while (true) {
                __sync_synchronize();
                cur_tail = tail;
                node->next = cur_tail->next;
                if (node->next == head) {
                    break;
                }
                if (cur_tail->next == cur_tail) {
                    tail = head;
                } else {
                    __sync_bool_compare_and_swap(&tail, cur_tail, node->next);
                }
            }
        } while (!__sync_bool_compare_and_swap(&tail->next, head, node));
    }

    ywQueue<DATA>  * pop() {
        ywQueue<DATA> *ret;
        ywQueue<DATA> *new_next;

        while (true) {
            __sync_synchronize();
            ret = head->next;
            new_next = ret->next;
            if (ret == head) {
                return NULL;
            }
            if (new_next == ret) {
                continue;
            }
            if (__sync_bool_compare_and_swap(&ret->next, new_next, ret)) {
                break;
            }
        }
        assert(__sync_bool_compare_and_swap(&head->next, ret, new_next));

        return ret;
    }

    size_t calc_count() {
        size_t k = 0;
        ywQueue<int32_t> * iter;

        for (iter = head->next; iter != head; iter = iter->next) {
            k++;
        }
        return k;
    }



 private:
    ywQueue<DATA> *head;
    ywQueue<DATA> *tail;
    ywQueue<DATA>  head_instance;

    friend class ywqTestClass;
};

void     ywq_test();


/*
template<typename DATA>
class ywQueueHead {
    ywQueue<DATA> retry_node;

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
            old_next = push_before(node);
        } while (is_retry(old_next));

        push_after(old_next, node);
    }
    ywQueue<DATA>  * pop() {
        ywQueue<DATA> * ret;
        ywQueue<DATA> * prev = NULL;
        ywQueue<DATA> * prev_prev = NULL;

        do {
            ret = pop_before(&prev, &prev_prev);
        } while (is_retry(ret));

        if (ret) {
            pop_after(prev, prev_prev);
        }
        return ret;
    }

    ywQueue<DATA> *getEnd() {
        return &head;
    }

    size_t calc_count() {
        size_t k = 0;
        ywQueue<int32_t> * iter;

        for (iter = head.next; iter != &head; iter = iter->next) {
            k++;
        }
        return k;
    }

 private:
    ywQueue<DATA> * push_before(ywQueue<DATA> *node) {
        ywQueue<DATA> * old_next;

        __sync_synchronize();
        old_next = head.next;
        node->next = old_next;
        node->prev = &head;
        if (old_next->prev != &head) {
            return &retry_node;
        }
        if (!__sync_bool_compare_and_swap(&head.next, old_next, node)) {
            return &retry_node;
        }

        return old_next;
    }

    void push_after(ywQueue<DATA> *old_next, ywQueue<DATA> *node) {
        while (!__sync_bool_compare_and_swap(&old_next->prev, &head, node)) {
        }
    }

    ywQueue<DATA>  * pop_before(ywQueue<DATA> **_prev,
                                ywQueue<DATA> **_prev_prev) {
        ywQueue<DATA> * prev;
        ywQueue<DATA> * prev_prev;

        __sync_synchronize();
        prev = head.prev;
        prev_prev = prev->prev;
        if (prev == &head) {
            return NULL;
        }
        if ((prev->next != &head) || (prev_prev->next != prev)) {
            return &retry_node;
        }
        if (!__sync_bool_compare_and_swap(&prev_prev->next, prev, &head)) {
            return &retry_node;
        }

        *_prev = prev;
        *_prev_prev = prev_prev;

        return prev;
    }

    bool is_retry(ywQueue<DATA> * node) {
        return node == &retry_node;
    }

    template<typename TYPE>
    inline static TYPE load(TYPE *target) {
            return *const_cast<volatile TYPE *>(target);
    }

    void pop_after(ywQueue<DATA> * prev, ywQueue<DATA> * prev_prev) {
        while (!__sync_bool_compare_and_swap(&head.prev, prev, prev_prev)) {
        }
    }

    ywQueue<DATA> head;

    friend class ywqTestClass;
};

void     ywq_test();
*/

#endif  // INCLUDE_YWQ_H_
