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

typedef uint64_t ref_slot;
typedef void**   rcu_ptr;

typedef struct {
    ref_slot  remove_time;
    void     *target;
} ywrcu_free_t;

typedef ywQueue<ywrcu_free_t> ywrcu_free_queue;


class ywRcuRef {
    static const int32_t  REF_COUNT  = MAX_THREAD_COUNT;
    static const ref_slot NIL_SLOT   = 0;
    static const ref_slot INIT_SLOT  = 2;
    static const ref_slot LOCK_BIT   = 1;
    static const uint32_t DEFAULT_TIMEOUT = 100000000;

 public:
    ywRcuRef() {
        int32_t i = 0;
        for (i = 0; i < REF_COUNT; ++i) {
            slot[i] = NIL_SLOT;
        }

        gslot = INIT_SLOT;
    }
    ~ywRcuRef() {
    }

    void fix() {
        *get_slot() = gslot;
    }
    void unfix() {
        *get_slot() = 0;
    }

    bool lock(uint32_t timeout = DEFAULT_TIMEOUT) {
        ref_slot  prev;
        uint32_t i;
        for (i = 0; i < timeout; ++i) {
            prev = gslot;
            if ((prev & LOCK_BIT) == 0) {
                if (__sync_bool_compare_and_swap(
                            &gslot, prev, prev | LOCK_BIT)) {
                    fix();
                    return true;
                }
            }
        }
        return false;
    }

    void release() {
        unfix();
        assert(gslot & LOCK_BIT);
        assert(__sync_bool_compare_and_swap(&gslot, gslot, gslot + 1));
    }

    void regist_free_obj(void *ptr) {
        ywrcu_free_queue *node;

        assert(gslot & LOCK_BIT);
        assert(*get_slot() == gslot);

        node = rc_free_pool.alloc();
        node->data.remove_time = gslot;
        node->data.target      = ptr;
        assert(is_reusable(&node->data) == false);
        free_q.push(node);
    }

    void *get_reusable_item() {
        ywrcu_free_queue *node = _get_reusable_item();
        void             *ret  = NULL;
        if (node) {
            ret = node->data.target;
            rc_free_pool.free_mem(node);
            return ret;
        }
        return NULL;
    }

    ywrcu_free_queue *_get_reusable_item() {
        ywrcu_free_queue *node = free_q.pop();
        if (node) {
            if (is_reusable(&(node->data))) {
                return node;
            }
            free_q.push(node);
        }
        return NULL;
    }

    ref_slot  get_global_ref() {
        return gslot;
    }

 private:
    bool is_reusable(ywrcu_free_t *free) {
        int32_t i;
        for (i = 0; i < REF_COUNT; ++i) {
            if (slot[i]) {
                if (slot[i] <= free->remove_time) {
                    return false;
                }
            }
        }

        return true;
    }

    volatile ref_slot *get_slot() {
        ywTID tid = ywThreadPool::get_thread_id();

        return &slot[tid];
    }

    volatile ref_slot    slot[REF_COUNT];
    volatile ref_slot    gslot;
    ywrcu_free_queue     free_q;

    static ywMemPool<ywrcu_free_queue>  rc_free_pool;
};

extern void rcu_ref_test();

#endif  // INCLUDE_YWRCUREF_H_
