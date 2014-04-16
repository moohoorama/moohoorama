/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWRCUREF_H_
#define INCLUDE_YWRCUREF_H_

#include <ywcommon.h>
#include <ywutil.h>
#include <ywworker.h>
#include <ywspinlock.h>
#include <ywq.h>
#include <ywmempool.h>
#include <ywaccumulator.h>
#include <ywtimer.h>
#include <ywpool.h>

class ywRcuRef;
class ywRcuRefManager;

typedef uint64_t ywr_time;
typedef int32_t  ref_count;
typedef void**   rcu_ptr;

typedef struct {
    ywDList    free_list;
    ywDList    sub_list;
    ywr_time   remove_time;
} ywrcu_free_ring;

typedef struct {
    volatile ywr_time             fixtime;
    ref_count                     ref;
    ywrcu_free_ring              *oldest_ring;
    ywrcu_free_ring               latest_ring;
} ywrcu_slot;

class ywRcuRefManager {
    static const int32_t  REUSE_INTERVAL = 1;

 public:
    explicit ywRcuRefManager() {
        ywTimer::get_instance()->regist(update, NULL, REUSE_INTERVAL);
    }
    inline static ywRcuRefManager *get_instance() {
        return &gInstance;
    }

    void regist(ywRcuRef *rcu);
    void unregist(ywRcuRef *rcu);
    void _update();

    static void update(void *arg);

    ywrcu_free_ring *alloc() {
        return free_ring_mempool.alloc();
    }
    void free(ywrcu_free_ring *ring) {
        free_ring_mempool.free_mem(ring);
    }

 private:
    ywMemPool<ywrcu_free_ring> free_ring_mempool;
    ywDList                    list;
    int32_t                    rcu_count;
    ywSpinLock                 lock;

    static ywRcuRefManager     gInstance;
};

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
            slot[i].oldest_ring = NULL;
            init_ring(&slot[i].latest_ring);
        }
        global_time   = INIT_TIME;
        reusable_time = INIT_TIME;
    }
    ~ywRcuRef() {
        free_all();
        ywRcuRefManager::get_instance()->unregist(this);
    }

    ywrcu_slot *fix() {
        ywrcu_slot *slot = get_slot();

        if (slot->fixtime) {
            slot->ref++;
        } else {
            slot->fixtime = global_time;
        }
        return slot;
    }

    void unfix(ywrcu_slot *slot) {
        if (!slot->ref) {
            slot->fixtime = 0;
        } else {
            slot->ref--;
        }
    }

    void unfix() {
        unfix(get_slot());
    }

    bool lock(uint32_t timeout = DEFAULT_TIMEOUT) {
        ywr_time    prev;
//        ywrcu_slot *slot = get_slot();
        uint32_t    i;

        if (local_list.is_unlinked()) {
            ywRcuRefManager::get_instance()->regist(this);
        }

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

    template<typename T>
    void regist_free_obj(T *ptr) {
        static_assert(sizeof(T) >= sizeof(ywDList), "Too small datatype");

        ywrcu_slot *slot = get_slot();
        ywDList    *target = reinterpret_cast<ywDList*>(ptr);

        if (slot->latest_ring.remove_time < global_time) {
            change_ring(slot);
        }
        slot->latest_ring.sub_list.attach(target);
    }

    template<typename T>
    T *get_reusable_item() {
        static_assert(sizeof(T) >= sizeof(ywDList), "Too small datatype");
        ywrcu_slot      *slot = get_slot();
        ywrcu_free_ring *node = slot->oldest_ring;

        while (true) {
            if (!node) {
                node = free_ring_pool.pop();
                if (!node) {
                    change_ring(slot);
                    node = free_ring_pool.pop();
                }
            }
            if (!node) return NULL;

            slot->oldest_ring = node;
            if (!is_reusable(node)) return NULL;

            T *ret = reinterpret_cast<T*>(node->sub_list.pop());
            if (ret) {
                return ret;
            }

            ywRcuRefManager::get_instance()->free(node);
            slot->oldest_ring = NULL;
            node = NULL;
        }
        return NULL;
    }

    void free_all() {
        ywrcu_free_ring *node;

        while ((node = free_ring_pool.pop())) {
            ywRcuRefManager::get_instance()->free(node);
        }
    }

    void aging() {
        ywrcu_slot      *slot = get_slot();

        change_ring(slot);
        update_reusable_time();
        free_ring_pool.reclaim();
    }

    size_t get_free_count() {
        return free_ring_pool.get_pooling_count();
    }

    ywr_time get_global_time() {
        return global_time;
    }

 private:
    void init_ring(ywrcu_free_ring *ring) {
        ring->free_list.init();
        ring->sub_list.init();
        ring->remove_time = -1;
    }

    void change_ring(ywrcu_slot *slot) {
        if (!slot->latest_ring.sub_list.is_unlinked()) {
            ywrcu_free_ring *ring;

            ring = ywRcuRefManager::get_instance()->alloc();
            ring->remove_time = global_time;
            ring->free_list.init();
            ring->sub_list.init();
            ring->sub_list.bring(&slot->latest_ring.sub_list);
            assert(slot->latest_ring.sub_list.is_unlinked());
            free_ring_pool.push(ring);
            init_ring(&slot->latest_ring);
        }
    }
    bool is_reusable(ywrcu_free_ring *free) {
        if (reusable_time > free->remove_time) {
            return true;
        } else {
            if (reusable_time > free->remove_time) {
                return true;
            }
        }
        return false;
    }

    void update_reusable_time() {
        ywr_time fix_time;
        ywr_time cur_time;
        int32_t  i;

        if (lock()) {
            cur_time = global_time;
            for (i = 0; i < SLOT_COUNT; ++i) {
                fix_time = slot[i].fixtime;
                if ((fix_time) &&
                    (fix_time < cur_time)) {
                    cur_time = fix_time;
                }
            }
            reusable_time = cur_time;
            release();
        }
    }

    ywrcu_slot *get_slot() {
        ywTID tid = ywWorkerPool::get_thread_id();

        return &slot[tid];
    }

    ywDList                           local_list;
    ywrcu_slot                        slot[SLOT_COUNT];
    volatile ywr_time                 global_time;
    volatile ywr_time                 reusable_time;
    ywPool<ywrcu_free_ring>           free_ring_pool;


    friend class ywRcuRefManager;
    friend void basic_test();
};

class ywRcuGuard {
 public:
    explicit ywRcuGuard(ywRcuRef *_target):target(_target) {
        slot = target->fix();
    }
    ~ywRcuGuard() {
        target->unfix(slot);
    }

 private:
    ywrcu_slot   *slot;
    ywRcuRef     *target;
};

extern void rcu_ref_test();

#endif  // INCLUDE_YWRCUREF_H_
