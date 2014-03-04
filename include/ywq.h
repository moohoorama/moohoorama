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
        count     = 0;
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
        ywQueue<DATA> * prev;
        ywQueue<DATA> * prev_prev;

        do {
            ret = pop_before(&prev, &prev_prev);
        } while (is_retry(ret));

        if (ret) {
            pop_after(prev, prev_prev);
        }
        return ret;
    }

    int32_t ptr2int(ywQueue<DATA> *ptr) {
        return reinterpret_cast<int32_t>(ptr);
    }
    void    print_ptr(ywQueue<DATA> *ptr) {
        printf(
            "- 0x%08x 0x%08x 0x%08x -",
            ptr2int(ptr->prev),
            ptr2int(ptr),
            ptr2int(ptr->next));
    }
    void draw() {
        ywQueue<DATA> * iter;
        int             i;

        iter = &head;
        printf("Count : %d\n", count);
        printf("NEXT : ");
        for (i = 0; i < 4; ++i) {
            print_ptr(iter);
            iter = iter->next;
            if (iter == &head) {
                break;
            }
        }
        printf("\n");

        iter = &head;
        printf("PREV : ");
        for (i = 0; i < 4; ++i) {
            print_ptr(iter);
            iter = iter->prev;
            if (iter == &head) {
                break;
            }
        }
        printf("\n");
        assert(false);
    }

    ywQueue<DATA> *getEnd() {
        return &head;
    }
    size_t get_count() {
        return count;
    }
    size_t calc_count() {
        size_t k = 0;
        ywQueue<int32_t> * iter;

        for (iter = head.next; iter != &head; iter = iter->next) {
            k++;
        }
        return k;
    }

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
        if (!__sync_bool_compare_and_swap(&old_next->prev, &head, node)) {
            printf("old_next : ");
            print_ptr(old_next);
            printf("\n");
            printf("node     : ");
            print_ptr(node);
            printf("\n");
            draw();
        }
        __sync_add_and_fetch(&count, 1);
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

    void pop_after(ywQueue<DATA> * prev, ywQueue<DATA> * prev_prev) {
        if (!__sync_bool_compare_and_swap(&head.prev, prev, prev_prev)) {
            printf("prev     : ");
            print_ptr(prev);
            printf("\n");
            printf("prev_prev: ");
            print_ptr(prev_prev);
            printf("\n");
            draw();
        }
        __sync_sub_and_fetch(&count, 1);
    }

 private:
    ywQueue<DATA> head;
    int           count;
};

void     ywq_test();

#endif  // INCLUDE_YWQ_H_
