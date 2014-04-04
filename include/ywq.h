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

template<typename DATA, bool sync = true>
class ywQueueHead {
 public:
    ywQueueHead():head(&head_instance) {
        init();
    }
    void init() {
        head->next = head;
        tail = head;
    }

    ywQueue<DATA> *get_head() {
        return head;
    }

    template<typename T>
    bool cas(T *ptr, T prev, T next) {
        if (sync)
            return __sync_bool_compare_and_swap(ptr, prev, next);
        *ptr = next;
        return true;
    }

    template<typename T>
    T *make_circle(T *ptr) {
        T * old_next = ptr->next;
        if (ptr == tail) {
            cas(&tail, tail, tail->next);
            return NULL;
        }
        if (old_next != ptr) {
            if (cas(&ptr->next, old_next, ptr)) {
                if (ptr == tail) {
                    cas(&tail, tail, tail->next);
                }
                return old_next;
            }
        }
        return NULL;
    }

    void synchronize() {
        if (sync)
            __sync_synchronize();
    }

    void push(ywQueue<DATA> *node) {
        ywQueue<DATA> *cur_tail;
        int32_t        retry = 0;

        do {
            while (true) {
                synchronize();
                cur_tail = tail;
                assert(tail->next);
                node->next = cur_tail->next;
                if (node->next == head) {
                    break;
                }
                if (cur_tail->next == cur_tail) {
                    tail = head;
                    assert(tail->next);
                } else {
                    cas(&tail, cur_tail, node->next);
                    assert(tail->next);
                }
                ++retry;
                if (retry > 1000000) {
                    dump();
                    retry = 0;
                }
            }
        } while (!cas(&tail->next, head, node));
    }

    ywQueue<DATA>  * pop() {
        ywQueue<DATA> *ret;
        ywQueue<DATA> *new_next;

        do {
            synchronize();
            ret = head->next;
            if (ret == head) {
                return NULL;
            }
        } while (!(new_next = make_circle(ret)));

        assert(cas(&head->next, ret, new_next));

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

    /*  src_q : -A-B-C-D- => dst_q.bring(A,C) => src_q : -A-D-
     *  dst_q : -E-F-                            dst_q : -B-C-E-F- */
    void bring(ywQueue<DATA> * _before_first, ywQueue<DATA> *_last) {
        assert(sync == false);

        ywQueue<DATA> * _first = _before_first->next;

        /* detach targets from src_1 */
        /* src_q : -A-B-C-D- => src_q : -A-D- */
        _before_first->next = _last->next;

        /*  dst_q :-E-F-  => -B-C-E-F- */
        _last->next = head->next;
        head->next  = _first;
    }

    void bring_all(ywQueueHead<DATA, sync> * src_q) {
        ywQueue<DATA> * iter;
        ywQueue<DATA> * s_head = src_q->head;

        for (iter = s_head->next; iter->next != s_head; iter = iter->next) {
        }
        bring(s_head, iter);
    }

    void dump() {
        ywQueue<DATA> * iter;
        intptr_t        ptr;

        printf("dump {");
        for (iter = head; iter->next != head; iter = iter->next) {
            ptr = reinterpret_cast<intptr_t>(iter);
            if (iter == tail) {
                printf("T[%08"PRIxPTR"] ", ptr);
            } else {
                printf(" [%08"PRIxPTR"] ", ptr);
            }
        }
        printf("}\n");
    }

 private:
    ywQueue<DATA> *head;
    ywQueue<DATA> *tail;
    ywQueue<DATA>  head_instance;

    friend class ywqTestClass;
};

void     ywq_test();

#endif  // INCLUDE_YWQ_H_
