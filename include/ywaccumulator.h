/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWACCUMULATOR_H_
#define INCLUDE_YWACCUMULATOR_H_

#include <ywcommon.h>
#include <ywworker.h>
#include <ywspinlock.h>
#include <ywq.h>

template<typename T, int32_t method = 0>
class ywAccumulator{
    static const int32_t  NULL_SLOT_IDX = -1;

    static const int32_t CHUNK_COUNT = 1024;
    static const int32_t SLOT_COUNT  = 64*CACHE_LINE_SIZE/sizeof(T);
    static const int32_t CHUNK_SIZE  = SLOT_COUNT*MAX_THREAD_COUNT/sizeof(T);

 public:
    explicit ywAccumulator(T _default_data = 0):default_data(_default_data) {
        assert(ywWorkerPool::get_instance()->get_max_thread_id()
               <= MAX_THREAD_COUNT);
        assert(MAX_THREAD_COUNT >= 2);
        assert(alloc_slot());
    }
    ~ywAccumulator() {
        assert(free_slot());
    }

    void mutate(T delta) {
        if (delta) {
            switch (method) {
                case 0:
                    *data[ywWorkerPool::get_thread_id()] += delta;
                    break;
                case 1:
                    __sync_add_and_fetch(data[0], delta);
                    break;
                case 2:
                    *(volatile T*)data[0] += delta;
                    break;
            }
        }
    }

    void set(T value) {
        switch (method) {
            case 0:
                *data[ywWorkerPool::get_thread_id()] = value;
                break;
            case 1:
            case 2:
                *(volatile T*)data[0] = value;
                break;
        }
    }


    void reset() {
        int32_t i;

        for (i = 0; i < MAX_THREAD_COUNT; ++i) {
            *data[i] = default_data;
        }
    }

    T sum() {
        int32_t i;
        T       val = 0;

        for (i = 0; i < MAX_THREAD_COUNT; ++i) {
            val += *data[i];
        }
        return val;
    }

    T min() {
        int32_t i;
        T       val = *data[0];

        assert(default_data != 0);

        for (i = 0; i < MAX_THREAD_COUNT; ++i) {
            if (*data[i] < val) {
                val = *data[i];
            }
        }
        return val;
    }

    T max() {
        int32_t i;
        T       val = data[0];

        for (i = 0; i < MAX_THREAD_COUNT; ++i) {
            if (*data[i] > val) {
                val = *data[i];
            }
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
        ywQueue<int32_t> * free_ptr;
        int32_t            i;

        if (!lock()) {
            return false;
        }
        free_ptr = free_list.pop();
        if (free_ptr) {
            T * base_ptr = reinterpret_cast<T*>(free_ptr);
            --free_count;

            for (i = 0; i < MAX_THREAD_COUNT; ++i) {
                data[i] = base_ptr + SLOT_COUNT*i;
            }
        } else {
            if (global_idx >= get_max_slot_count()) {
                release();
                return false;
            }

            int32_t slot_idx =  global_idx++;
            if (!get_chunk(slot_idx)) {
                root[slot_idx / SLOT_COUNT] =
                    new T[SLOT_COUNT*MAX_THREAD_COUNT];
            }

            for (i = 0; i < MAX_THREAD_COUNT; ++i) {
                data[i] = get_ptr(i, slot_idx);
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
        ywQueue<int32_t> * ptr;

        if (!lock()) {
            return false;
        }
        ++free_count;
        ptr = reinterpret_cast<ywQueue<int32_t>*>(data[0]);
        assert(ptr);
        free_list.push(ptr);
#ifdef REPORT
        report();
#endif
        release();
        return true;
    }

    static bool lock() {
        return g_acc_lock.WLock();
    }
    static void release() {
        g_acc_lock.release();
    }

    static T           *get_chunk(int32_t _idx) {
        int32_t chunk_idx = _idx / SLOT_COUNT;
        return root[chunk_idx];
    }
    static T           *get_ptr(int32_t tid, int32_t _idx) {
        return get_chunk(_idx) + (_idx % SLOT_COUNT) + tid*SLOT_COUNT;
    }

    T                                  *data[MAX_THREAD_COUNT];
    T                                   default_data;
    static ywSpinLock                   g_acc_lock;
    static T                           *root[CHUNK_COUNT];
    static int32_t                      global_idx;
    static int32_t                      free_count;
    static ywQueueHead<int32_t, false>  free_list;
};

extern void accumulator_test();
extern void atomic_stat_test();
extern void dirty_stat_test();

#endif  // INCLUDE_YWACCUMULATOR_H_
