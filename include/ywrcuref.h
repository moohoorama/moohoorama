/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWRCUREF_H_
#define INCLUDE_YWRCUREF_H_

#include <ywcommon.h>
#include <ywutil.h>
#include <ywthread.h>
#include <ywspinlock.h>
#include <ywq.h>
#include <ywmempool.h>
#include <ywaccumulator.h>

typedef uint64_t ywr_time;
typedef int32_t  ref_count;
typedef void**   rcu_ptr;

typedef struct {
    ywr_time  remove_time;
    void     *target;
} ywrcu_free_t;


typedef ywQueue<ywrcu_free_t> ywrcu_free_queue;

typedef struct {
    volatile ywr_time   fixtime;
    ref_count           ref;
    ywrcu_free_queue   *oldest_free;
} ywrcu_slot;



class ywRcuRef {
    static const int32_t  SLOT_COUNT  = MAX_THREAD_COUNT;
    static const ywr_time NIL_TIME   = 0;
    static const ywr_time INIT_TIME  = 2;
    static const ywr_time LOCK_BIT   = 1;
    static const uint32_t DEFAULT_TIMEOUT = 100000000;

 public:
    ywRcuRef() {
        for (int32_t i = 0; i < SLOT_COUNT; ++i) {
            slot[i].fixtime     = NIL_TIME;
            slot[i].ref         = 0;
            slot[i].oldest_free = NULL;
        }
        global_time = INIT_TIME;
    }
    ~ywRcuRef() {
        free_all();
    }

    void fix() {
        ywrcu_slot *slot = get_slot();

        if (slot->fixtime) {
            slot->ref++;
        } else {
            slot->fixtime = global_time;
        }
    }
    void unfix() {
        ywrcu_slot *slot = get_slot();

        if (!slot->ref) {
            slot->fixtime = 0;
        } else {
            slot->ref--;
        }
    }

    bool lock(uint32_t timeout = DEFAULT_TIMEOUT) {
        ywr_time  prev;
        uint32_t i;
        for (i = 0; i < timeout; ++i) {
            prev = global_time;
            if (!(prev & LOCK_BIT)) {
                if (__sync_bool_compare_and_swap(
                            &global_time, prev, prev | LOCK_BIT)) {
                    return true;
                }
            }
        }
        return false;
    }

    void release() {
        assert(global_time & LOCK_BIT);
        assert(__sync_bool_compare_and_swap(
                &global_time, global_time, global_time + 1));
    }

    void regist_free_obj(void *ptr) {
        ywrcu_free_queue *node;

        assert(global_time & LOCK_BIT);

        node = rc_free_pool.alloc();
        node->data.remove_time = global_time;
        node->data.target      = ptr;
        assert(is_reusable(&node->data) == false);
        free_q.push(node);
    }

    void *get_reusable_item() {
        ywrcu_free_queue *node = _get_reusable_item();
        if (node) {
            void *ret = node->data.target;
            rc_free_pool.free_mem(node);
            return ret;
        }
        return NULL;
    }

    ywrcu_free_queue *_get_reusable_item() {
        ywrcu_slot       *slot = get_slot();
        ywrcu_free_queue *node = slot->oldest_free;
        if (!node) {
            node = free_q.pop();
        }
        if (node) {
            if (is_reusable(&(node->data))) {
                slot->oldest_free = NULL;
                return node;
            }
            slot->oldest_free = node;
        }
        return NULL;
    }

    void free_all() {
        ywrcu_free_queue *node;
        while ((node = free_q.pop())) {
            rc_free_pool.free_mem(node);
        }
    }

 private:
    bool is_reusable(ywrcu_free_t *free) {
        int32_t  i;
        ywr_time fix_time;
        if (global_time <= free->remove_time) {
            return false;
        }
        for (i = 0; i < SLOT_COUNT; ++i) {
            fix_time = slot[i].fixtime;
            if (fix_time) {
                if (fix_time <= free->remove_time) {
                    return false;
                }
            }
        }

        return true;
    }

    ywrcu_slot *get_slot() {
        ywTID tid = ywThreadPool::get_thread_id();

        return &slot[tid];
    }

    ywrcu_slot                 slot[SLOT_COUNT];
    volatile ywr_time          global_time;
    ywQueueHead<ywrcu_free_t>  free_q;

    static ywMemPool<ywrcu_free_queue> rc_free_pool;
};

extern void rcu_ref_test();

#endif  // INCLUDE_YWRCUREF_H_
