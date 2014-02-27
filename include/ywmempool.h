/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWMEMPOOL_H_
#define INCLUDE_YWMEMPOOL_H_

#include <ywcommon.h>
#include <ywdlist.h>
#include <ywthread.h>
#include <ywspinlock.h>
#include <string.h>
#include <stdlib.h>

#include <typeinfo>

template<typename T, size_t chunk_size =  64*KB>
class ywMemPool {
    static const size_t  UNIT = sizeof(T);
    static const size_t  PRIVATE_ALLOC_THRESHOLD = UNIT * 128;
    static const size_t  PRIVATE_FREE_THRESHOLD = UNIT * 256;

    static const size_t  CHUNK_MAX_COUNT = 1*GB / chunk_size;

    static const int32_t SHARED_IDX = MAX_THREAD_COUNT;
    static const int32_t MAX_IDX    = MAX_THREAD_COUNT+1;


    enum {
        CHUNK_STAT,
        FREE_STAT,
        MOVE_TO_SHARED_STAT,
        MOVE_FROM_SHARED_STAT,
        STAT_COUNT
    };

 public:
    explicit ywMemPool() {
        init();
    }
    ~ywMemPool() {
        report();
        free_all();
    }

    inline T *  alloc() {
        int32_t     tid = ywThreadPool::get_thread_id();
        ywdl_t    * free_ptr;

        while (true) {
            free_ptr = ywdl_pop(&free_list[tid]);
            if (free_ptr) {
                size[FREE_STAT][tid] -= UNIT;
                return reinterpret_cast<T*>(free_ptr);
            }
            if (!alloc_chunk(tid)) {
                return NULL;
            }
        }
    }
    inline void free_mem(T * mem) {
        ywdl_t * ptr = reinterpret_cast<ywdl_t*>(mem);
        int32_t  tid = ywThreadPool::get_thread_id();
        size[FREE_STAT][tid] += UNIT;
        ywdl_attach(&free_list[tid], ptr);
        if (size[FREE_STAT][tid] >= PRIVATE_FREE_THRESHOLD) {
            shared_area_lock.WLock();
            ywdl_move_all(&free_list[tid], &free_list[SHARED_IDX]);
            size[FREE_STAT][SHARED_IDX] += size[FREE_STAT][tid];
            shared_area_lock.release();
            size[MOVE_TO_SHARED_STAT][tid] += size[FREE_STAT][tid];
            size[FREE_STAT][tid] = 0;
        }
    }

    void report() {
        int32_t i;
        int32_t j;
        size_t  stat[STAT_COUNT] = {0};

        for (i = 0; i < MAX_IDX; ++i) {
            for (j = 0; j < STAT_COUNT; ++j) {
                stat[j] += size[j][i];
            }
        }
        printf("%s\n", typeid(this).name());
        printf("CHUNK    %12dB %8dK %dM\n", stat[0], stat[0]/KB, stat[0]/MB);
        printf("FREE     %12dB %8dK %dM\n", stat[1], stat[1]/KB, stat[1]/MB);
        printf("TO_SHA   %12dB %8dK %dM\n", stat[2], stat[2]/KB, stat[2]/MB);
        printf("FROM_SHA %12dB %8dK %dM\n", stat[3], stat[3]/KB, stat[3]/MB);
    }

 private:
    void init() {
        static_assert(UNIT <= chunk_size, "Too small chunk");
        static_assert(sizeof(ywdl_t) <= UNIT, "Too small type");

        chunk_idx = 0;
        memset(size, 0, sizeof(size));
    }

    void free_all() {
        int32_t   i;

        shared_area_lock.WLock();
        for (i = 0; i < chunk_idx; ++i) {
            free(chunk_array[i]);
            size[CHUNK_STAT][i] -= chunk_size;
        }
        shared_area_lock.release();
    }

    bool move_from_shared_area(int tid) {
        int32_t  move_count = 0;
        if (size[FREE_STAT][SHARED_IDX] > PRIVATE_ALLOC_THRESHOLD) {
            shared_area_lock.WLock();
            if (size[FREE_STAT][SHARED_IDX] > PRIVATE_ALLOC_THRESHOLD) {
                move_count = PRIVATE_ALLOC_THRESHOLD / UNIT;
                ywdl_move_n(&free_list[SHARED_IDX],
                            &free_list[tid],
                            move_count);
                size[FREE_STAT][SHARED_IDX] -= PRIVATE_ALLOC_THRESHOLD;
                size[FREE_STAT][tid] += PRIVATE_ALLOC_THRESHOLD;
                size[MOVE_FROM_SHARED_STAT][tid] += PRIVATE_ALLOC_THRESHOLD;
            }
            shared_area_lock.release();
        }
        return move_count > 0;
    }

    bool alloc_chunk(int tid) {
        if (move_from_shared_area(tid))
            return true;
        char      * chunk = reinterpret_cast<char*>(malloc(chunk_size));
        ywdl_t    * ptr;
        size_t      i;

        if (!chunk) {
            return false;
        }

        for (i = 0; i + UNIT <= chunk_size; i += UNIT) {
            ptr = reinterpret_cast<ywdl_t*>(chunk+i);
            ywdl_attach(&free_list[tid], ptr);
            size[FREE_STAT][tid] += UNIT;
        }
        shared_area_lock.WLock();
        chunk_array[ chunk_idx++ ] = chunk;
        shared_area_lock.release();
        size[CHUNK_STAT][tid] += chunk_size;
        return true;
    }

    size_t     size[STAT_COUNT][MAX_IDX];
    char      *chunk_array[CHUNK_MAX_COUNT];
    int32_t    chunk_idx;
    ywdl_t     free_list[MAX_IDX];
    ywSpinLock shared_area_lock;
};

#endif  // INCLUDE_YWMEMPOOL_H_
