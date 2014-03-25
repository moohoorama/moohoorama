/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#ifndef INCLUDE_YWSL_H_
#define INCLUDE_YWSL_H_

#include <ywcommon.h>
#include <ywspinlock.h>

class ywDList;
class ywPDSlot;
template<typename T>
class ywParallelDList;

class ywDList {
 public:
    ywDList():next(this), prev(this) {
    }
    ywDList    * next;
    ywDList    * prev;

    void link(ywDList * target) {
        next         = target;
        target->prev = this;
    }

    void attach(ywDList * target) {
        target->link(next);
        this->link(target);
    }

    void detach() {
        prev->link(next);
    }

    ywDList * pop() {
        ywDList * ret = next;
        if (ret == this) {
            return NULL;
        }
        next->detach();

        return ret;
    }

    void bring(ywDList * head, ywDList * tail) {
        ywDList * before_head = head->prev;
        ywDList * after_tail = tail->next;

        /*attach*/
        tail->link(next);
        this->link(head);

        /*detach*/
        before_head->link(after_tail);
    }
};

class ywPDSlot {
 public:
    explicit ywPDSlot() {
        count = 0;
    }

    void push(ywDList *target) {
        list.attach(target);
        ++count;
    }

    ywDList *pop() {
        ywDList *ret = list.pop();
        if (ret) {
            assert(count > 0);
            --count;
        }
        return ret;
    }

    void validation() {
        ywDList *iter = list.next;
        size_t   i = 0;
        while (iter != &list) {
            ++i;
            iter = iter->next;
        }
        assert(i == count);
    }

    void bring(ywPDSlot *owner, ywDList *head, ywDList *tail, int32_t _count) {
        if (_count) {
            list.bring(head, tail);
            this->count  += _count;
            owner->count -= _count;
        }
    }

    void bring(ywPDSlot *owner, int32_t _count) {
        ywDList *head;
        ywDList *tail;
        int32_t  i;

        head = owner->list.next;
        tail = &owner->list;
        for (i = 0; i < _count; ++i) {
            if (tail == owner->list.prev) {
                break;
            }
            tail = tail->next;
        }
        bring(owner, head, tail, i);
        this->validation();
    }

    ywDList list;
    size_t  count;
} __attribute__((aligned(CACHE_LINE_SIZE)));

template<typename T>
class ywParallelDList {
    static const size_t  SHARING_THRESHOLD = 256;
    static const size_t  SHARING_COUNT = 256;
    static const size_t  BEGGING_COUNT = 256;
    static const int32_t SHARED_IDX = MAX_THREAD_COUNT;
    static const int32_t MAX_IDX    = MAX_THREAD_COUNT+1;
    static const int32_t PRINT_WIDTH = 8;

 public:
    ywParallelDList() {
        static_assert(sizeof(T) >= sizeof(ywDList), "Too small object");
    }
    ~ywParallelDList() {
        int32_t i = 0;
        for (i = 0; i < MAX_IDX; ++i) {
            if (slot[i].count) {
                print();
                assert(slot[i].count == 0);
            }
        }
    }

    void push(T * _target) {
        ywPDSlot * private_slot = get_private_slot();
        ywDList  * target = reinterpret_cast<ywDList*>(_target);

        private_slot->push(target);
        if (private_slot->count > SHARING_THRESHOLD) {
            move_to_shared_area(private_slot);
        }
    }

    void move_to_shared_area(ywPDSlot *private_slot) {
        ywPDSlot *shared_slot = get_shared_slot();
        ywDList  *head;
        ywDList  *tail;
        size_t   i;

        head = private_slot->list.next;
        tail = &private_slot->list;
        for (i = 0; i < SHARING_COUNT; ++i) {
            if (tail == private_slot->list.prev) {
                break;
            }
            tail = tail->next;
        }
        if (lock.tryWLock()) {
            shared_slot->bring(private_slot, head, tail, i);
            lock.release();
        }
    }

    T *pop() {
        ywPDSlot * private_slot = get_private_slot();
        ywDList  * ret = private_slot->pop();

        if (!ret) {
            if (bring_from_shared_area(private_slot)) {
                ret = private_slot->pop();
            }
        }
        return reinterpret_cast<T*>(ret);
    }

    bool bring_from_shared_area(ywPDSlot *private_slot) {
        ywPDSlot *shared_slot = get_shared_slot();

        if (lock.tryWLock()) {
            private_slot->bring(shared_slot, BEGGING_COUNT);
            lock.release();
            return true;
        }
        return false;
    }

    void print() {
        int32_t i = 0;
        int32_t j = 0;
        int64_t total = 0;

        printf("%6s ", "");
        for (j = 0; j < PRINT_WIDTH; ++j) {
            printf("%6d ", j);
        }
        printf("\n");

        for (i = 0; i < SHARED_IDX; i += PRINT_WIDTH) {
            for (j = 0; (j < PRINT_WIDTH) && (i < MAX_IDX); ++j) {
                if (slot[i+j].count) {
                    break;
                }
            }
            if (j != PRINT_WIDTH) {
                printf("%6d ", i);
                for (j = 0; (j < PRINT_WIDTH) && (i < MAX_IDX); ++j) {
                    printf("%6"PRId64" ", (int64_t)slot[i+j].count);
                    total += slot[i+j].count;
                }
                printf("\n");
            }
        }
        total += slot[SHARED_IDX].count;
        printf("%6s %6"PRId64"    %6s %6"PRId64"\n\n",
               "SHARED", (int64_t)slot[SHARED_IDX].count,
               "TOTAL", total);
    }

 private:
    ywPDSlot *get_shared_slot() {
        return &slot[SHARED_IDX];
    }

    ywPDSlot *get_private_slot() {
        int32_t tid = ywThreadPool::get_thread_id();

        return &slot[tid];
    }

    ywPDSlot    slot[MAX_IDX];
    ywSpinLock  lock;
};

extern void ywParallelDListTest();
extern void ywParallelDListCCTest(int32_t num);

#endif  // INCLUDE_YWSL_H_

