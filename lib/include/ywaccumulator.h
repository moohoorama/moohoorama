/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWACCUMULATOR_H_
#define INCLUDE_YWACCUMULATOR_H_

#include <ywcommon.h>
#include <ywthread.h>
#include <ywspinlock.h>
#include <ywsll.h>

template<typename T, bool use_atomic>
class ywAccumulator{
    static const int32_t  NULL_SLOT_IDX = -1;

    static const int32_t CHUNK_COUNT = 1024;
    static const int32_t SLOT_COUNT  = 64*CACHE_LINE_SIZE/sizeof(T);
    static const int32_t CHUNK_SIZE  = SLOT_COUNT*MAX_THREAD_COUNT/sizeof(T);

 public:
    ywAccumulator() {
        assert(ywThreadPool::get_instance()->get_max_thread_id()
               <= MAX_THREAD_COUNT);
        assert(MAX_THREAD_COUNT >= 2);
        assert(alloc_slot());
    }
    ~ywAccumulator() {
        assert(free_slot());
    }

    void mutate(T delta) {
        if (use_atomic == true) {
            __sync_add_and_fetch(get_ptr(0, slot_idx), delta);
        } else {
            *(get_ptr(ywThreadPool::get_thread_id(), slot_idx)) += delta;
        }
    }

    void reset() {
        int32_t i;

        for (i = 0; i < MAX_THREAD_COUNT; ++i) {
            *(get_ptr(i, slot_idx)) = 0;
        }
    }

    T get() {
        int32_t i;
        T       val = 0;

        for (i = 0; i < MAX_THREAD_COUNT; ++i) {
            val += *(get_ptr(i, slot_idx));
        }
        return val;
    }

    int32_t get_max_slot_count() {
        return CHUNK_COUNT * SLOT_COUNT;
    }

    void report() {
        int32_t chunk_count;
        printf("Accumulator Global Stat:\n");
        printf("global_idx : %d\n", global_idx);
        printf("free_count : %d\n", free_count);
        chunk_count =
            (global_idx+SLOT_COUNT-1)/SLOT_COUNT;
        printf("memory_size: %d(%dKB %dMB)\n\n",
               chunk_count * CHUNK_SIZE,
               chunk_count * CHUNK_SIZE/1024,
               chunk_count * CHUNK_SIZE/1024/1024);
    }

 private:
    bool alloc_slot() {
        ywsllNode * free_ptr;
        if (!lock()) {
            return false;
        }
        free_ptr = ywsll_pop(&free_list);
        if (free_ptr) {
            --free_count;
            slot_idx = *((reinterpret_cast<T*>(free_ptr))+SLOT_COUNT);
        } else {
            if (global_idx >= get_max_slot_count()) {
                release();
                return false;
            }

            slot_idx =  global_idx++;
            if (!get_chunk(slot_idx)) {
                root[slot_idx / SLOT_COUNT] =
                    new T[SLOT_COUNT*MAX_THREAD_COUNT];
            }
        }
        reset();
#ifdef REPORT
        report();
#endif

        release();
        return true;
    }
    bool free_slot() {
        ywsllNode * ptr;
        if (!lock()) {
            return false;
        }
        ++free_count;
        ptr = reinterpret_cast<ywsllNode*>(get_ptr(0, slot_idx));
        *get_ptr(1, slot_idx) = slot_idx;
        ywsll_attach(&free_list, ptr);
#ifdef REPORT
        report();
#endif
        release();
        return true;
    }

    static bool lock();
    static void release();

    static T           *get_chunk(int32_t _idx) {
        int32_t chunk_idx = _idx / SLOT_COUNT;
        return root[chunk_idx];
    }
    static T           *get_ptr(int32_t tid, int32_t _idx) {
        return get_chunk(_idx) + (_idx % SLOT_COUNT) + tid*SLOT_COUNT;
    }

    int32_t             slot_idx;

    static T           *root[CHUNK_COUNT];
    static int32_t      global_idx;
    static int32_t      free_count;
    static ywsllNode    free_list;
};

extern void accumulator_test();
extern void atomic_stat_test();

#endif  // INCLUDE_YWACCUMULATOR_H_
