/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#ifndef INCLUDE_YWPOOL_H_
#define INCLUDE_YWPOOL_H_

#include <ywsl.h>
#include <ywspinlock.h>

class ywPoolSlot {
 public:
    explicit ywPoolSlot() {
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

    void bring(ywPoolSlot *own, ywDList *head, ywDList *tail, int32_t _count) {
        if (_count) {
            list.bring(head, tail);
            this->count += _count;
            own->count  -= _count;
        }
    }

    void bring(ywPoolSlot *own, int32_t _count) {
        ywDList *head;
        ywDList *tail;
        int32_t  i;

        head = own->list.next;
        tail = &own->list;
        for (i = 0; i < _count; ++i) {
            if (tail == own->list.prev) {
                break;
            }
            tail = tail->next;
        }
        bring(own, head, tail, i);
        this->validation();
    }

    ywDList list;
    size_t  count;
} __attribute__((aligned(CACHE_LINE_SIZE)));

template<typename T, size_t PRIVATE_COUNT = 128, size_t SHARING_THRESHOLD = 192>
class ywPool {
    static const int32_t SHARED_IDX = MAX_THREAD_COUNT;
    static const int32_t MAX_IDX    = MAX_THREAD_COUNT+1;
    static const int32_t PRINT_WIDTH = 8;

 public:
    ywPool() {
        static_assert(PRIVATE_COUNT <= SHARING_THRESHOLD,
                      "Invalid Pool configuration");
        static_assert(sizeof(T) >= sizeof(ywDList), "Too small object");
    }
    ~ywPool() {
    }

    void push(T * _target) {
        if (PRIVATE_COUNT == 0) {
            ywPoolSlot * shared_slot = get_shared_slot();
            ywDList    * target = reinterpret_cast<ywDList*>(_target);
            shared_slot->push(target);
        } else {
            ywPoolSlot * private_slot = get_private_slot();
            ywDList    * target = reinterpret_cast<ywDList*>(_target);

            private_slot->push(target);
            if (private_slot->count > SHARING_THRESHOLD) {
                move_to_shared_area(private_slot);
            }
        }
    }

    void move_to_shared_area(ywPoolSlot *private_slot) {
        ywPoolSlot *shared_slot = get_shared_slot();
        ywDList    *head;
        ywDList    *tail;
        size_t     i;

        head = private_slot->list.next;
        tail = &private_slot->list;
        for (i = 0; i < PRIVATE_COUNT; ++i) {
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
        ywDList    * ret = NULL;
        if (PRIVATE_COUNT == 0) {
            ywPoolSlot * shared_slot = get_shared_slot();
            ret = shared_slot->pop();
        } else {
            ywPoolSlot * private_slot = get_private_slot();
            ret = private_slot->pop();

            if (!ret) {
                if (bring_from_shared_area(private_slot)) {
                    ret = private_slot->pop();
                }
            }
        }
        return reinterpret_cast<T*>(ret);
    }

    bool bring_from_shared_area(ywPoolSlot *private_slot) {
        ywPoolSlot *shared_slot = get_shared_slot();

        if (lock.tryWLock()) {
            private_slot->bring(shared_slot, PRIVATE_COUNT);
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
    ywPoolSlot *get_shared_slot() {
        return &slot[SHARED_IDX];
    }

    ywPoolSlot *get_private_slot() {
        int32_t tid = ywThreadPool::get_thread_id();

        return &slot[tid];
    }

    ywPoolSlot    slot[MAX_IDX];
    ywSpinLock  lock;
};

extern void ywPoolTest();
extern void ywPoolCCTest(int32_t num);

#endif  // INCLUDE_YWPOOL_H_
