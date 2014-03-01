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
    int32_t   idx;
    void     *target;
} ywrcu_free_t;

typedef ywQueue<ywrcu_free_t> ywrcu_free_queue;

//   template<int32_t SLOT_COUNT = 1024>
class ywRcuRef {
    static const int32_t  SLOT_COUNT = 1024;
    static const int32_t  REF_COUNT  = MAX_THREAD_COUNT;
    static const ref_slot NIL_SLOT   = 0;
    static const ref_slot INIT_SLOT  = 2;
    static const ref_slot LOCK_BIT   = 1;
    static const uint32_t DEFAULT_TIMEOUT = 100000000;

 public:
    ywRcuRef() {
        int32_t i = 0;

        if (this == &global_refs) {
            for (i = 0; i < SLOT_COUNT; ++i) {
                slot[i] = INIT_SLOT;
            }
        } else {
            for (i = 0; i < SLOT_COUNT; ++i) {
                slot[i] = NIL_SLOT;
            }
        }
    }
    ~ywRcuRef() {
    }

    static void *fix(rcu_ptr ptr) {
        getInstance()->_fix(ptr);
        __sync_synchronize();
        return *const_cast<volatile rcu_ptr>(ptr);
    }
    static void unfix(rcu_ptr ptr) {
        getInstance()->_unfix(ptr);
    }

    static bool set(rcu_ptr ptr,
                    void* new_obj,
                    uint32_t timeout = DEFAULT_TIMEOUT) {
        return getInstance()->_set(ptr, new_obj, timeout);
    }

    static bool lock(rcu_ptr ptr, uint32_t timeout = DEFAULT_TIMEOUT) {
        return getInstance()->_lock(ptr, timeout);
    }

    static void release(rcu_ptr ptr) {
        getInstance()->_release(ptr);
    }


    static int32_t get_free_count() {
        int32_t i;
        int32_t ret = 0;
        for (i = 0; i < REF_COUNT; ++i) {
            ret += refs[i].free_count;
        }
        return ret;
    }

    static int32_t get_slot_idx(rcu_ptr ptr) {
        return simple_hash(sizeof(void*), reinterpret_cast<char*>(&ptr))
            % SLOT_COUNT;
    }

    static ref_slot get_slot(rcu_ptr ptr) {
        return global_refs.slot[get_slot_idx(ptr)];
    }
    static ref_slot get_fix_slot(rcu_ptr ptr) {
        return getInstance()->slot[get_slot_idx(ptr)];
    }

    /*
    static void *get_freeable_item() {
        ywrcu_free_queue *node = _get_freeable_item();
        void             *ret  = NULL;
        if (node) {
            ret = node->data.target;
            rc_free_pool.free_mem(node);
            return ret;
        }
        return NULL;
    }
    */
    static ywrcu_free_queue *_get_freeable_item() {
        ywrcu_free_queue *node = free_q.pop();
        ywTID             tid = ywThreadPool::get_thread_id();

        if (node) {
            if (is_freeable(&(node->data))) {
                --refs[tid].free_count;
                return node;
            }
            free_q.push(node);
        }
        return NULL;
    }

 private:
    static bool is_freeable(ywrcu_free_t *free) {
        int32_t  idx = free->idx;
        ref_slot remove_time =free->remove_time;
        ref_slot fix_time;
        int32_t  i = 0;

        if (global_refs.slot[idx] <= remove_time) {
            return false;
        }

        for (i = 0; i < REF_COUNT; ++i) {
            fix_time = refs[i].slot[idx];
            if (fix_time) {
                if (fix_time <= remove_time) {
                    /*
                    printf("[%d %d] %lld < %lld\n",
                            i, idx, fix_time, remove_time);
                            */
                    return false;
                }
            }
        }
        return true;
    }

    void _fix(rcu_ptr ptr) {
        int32_t idx = get_slot_idx(ptr);
        slot[idx] = global_refs.slot[idx];
    }

    void _unfix(rcu_ptr ptr) {
        int32_t idx = get_slot_idx(ptr);
        slot[idx] = 0;
    }

    bool _set(rcu_ptr ptr, void* new_obj, uint32_t timeout) {
        ywrcu_free_queue * node;

        int32_t idx = get_slot_idx(ptr);
        if (!_lock(ptr, timeout)) {
            return false;
        }

        node = rc_free_pool.alloc();
        node->data.remove_time = global_refs.slot[idx];
        node->data.idx         = idx;
        node->data.target      = *ptr;
        assert(is_freeable(&(node->data)) == false);
        free_q.push(node);
        *ptr = new_obj;
        ++free_count;
        _release(ptr);

        return true;
    }

    bool _lock(rcu_ptr ptr, uint32_t timeout) {
        int32_t  idx = get_slot_idx(ptr);
        ref_slot prev;
        uint32_t i;
        for (i = 0; i < timeout; ++i) {
            prev = global_refs.slot[idx];
            if ((prev & LOCK_BIT) == 0) {
                if (__sync_bool_compare_and_swap(
                    &global_refs.slot[idx], prev, prev | LOCK_BIT)) {
                    return true;
                }
            }
        }
        return false;
    }

    void _release(rcu_ptr ptr) {
        int32_t  idx = get_slot_idx(ptr);
        ref_slot prev = global_refs.slot[idx];

        assert(prev & LOCK_BIT);
        assert(__sync_bool_compare_and_swap(
            &global_refs.slot[idx], prev, prev+1));
    }

    static ywRcuRef *getInstance() {
        ywTID tid = ywThreadPool::get_thread_id();

        return const_cast<ywRcuRef*>(&refs[tid]);
    }

    volatile ref_slot                   slot[SLOT_COUNT];
    int32_t                             free_count;

    static ywrcu_free_queue             free_q;
    static volatile ywRcuRef            refs[REF_COUNT];
    static volatile ywRcuRef            global_refs;
    static ywMemPool<ywrcu_free_queue>  rc_free_pool;
};

extern void rcu_ref_test();

#endif  // INCLUDE_YWRCUREF_H_
