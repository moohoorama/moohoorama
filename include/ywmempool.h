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

    static const size_t  CHUNK_MAX_COUNT =   2*GB / chunk_size;

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
        const   char names[][10]={"CHUNK", "FREE", "TO_SHA", "FROM_SHA"};

        for (i = 0; i < MAX_IDX; ++i) {
            for (j = 0; j < STAT_COUNT; ++j) {
                stat[j] += size[j][i];
            }
        }
        printf("%s\n", typeid(this).name());
        for (i = 0; i < STAT_COUNT; ++i) {
            printf("%10s %12" PRIdPTR "B %8" PRIdPTR "K %" PRIdPTR "M\n",
                   names[i], stat[i], stat[i]/KB, stat[i]/MB);
        }
    }

    size_t get_size() {
        return size[CHUNK_STAT][SHARED_IDX];
    }

    int32_t get_chunk_count() {
        return chunk_idx;
    }

 private:
    void init() {
        static_assert(UNIT <= chunk_size, "Too small chunk");
        static_assert(sizeof(ywdl_t) <= UNIT, "Too small type");

        chunk_idx = 0;
        memset(size, 0, sizeof(size));
    }

    void free_all() {
        size_t   i;

        shared_area_lock.WLock();
        for (i = 0; i < chunk_idx; ++i) {
            free(chunk_array[i]);
            size[CHUNK_STAT][SHARED_IDX] -= chunk_size;
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
        if (chunk_idx >= CHUNK_MAX_COUNT) {
            return false;
        }
        char      * chunk = reinterpret_cast<char*>(malloc(chunk_size));
        ywdl_t    * ptr;
        size_t      i;

        if (!chunk) {
            return false;
        }

        shared_area_lock.WLock();
        if (chunk_idx >= CHUNK_MAX_COUNT) {
            shared_area_lock.release();
            free(chunk);
            return false;
        }
        chunk_array[ chunk_idx++ ] = chunk;
        size[CHUNK_STAT][SHARED_IDX] += chunk_size;
        shared_area_lock.release();


        for (i = 0; i + UNIT <= chunk_size; i += UNIT) {
            ptr = reinterpret_cast<ywdl_t*>(chunk+i);
            ywdl_attach(&free_list[tid], ptr);
            size[FREE_STAT][tid] += UNIT;
        }
        return true;
    }

    size_t     size[STAT_COUNT][MAX_IDX];
    char      *chunk_array[CHUNK_MAX_COUNT];
    size_t     chunk_idx;
    ywdl_t     free_list[MAX_IDX];
    ywSpinLock shared_area_lock;
};

template <typename T, int32_t MAX_COUNT = 16>
class ywMemPoolGuard {
 public:
    explicit ywMemPoolGuard(ywMemPool<T> *_pool):pool(_pool), count(0) {
    }

    ~ywMemPoolGuard() {
        rollback();
    }

    T *alloc() {
        T * ret = pool->alloc();
        if (ret) {
            if (regist(ret)) {
                return ret;
            }
            pool->free_mem(ret);
        }
        return NULL;
    }

    bool regist(T * target) {
        if (count >= MAX_COUNT) {
            return false;
        }
        mems[count++] = target;
        return true;
    }

    int32_t get_count() {
        return count;
    }

    void commit() {
        count = 0;
    }

    void rollback() {
        if (count > 0) {
            while (count--) {
                pool->free_mem(mems[count]);
            }
        }
    }

//  private:
    ywMemPool<T> *pool;
    int32_t       count;
    T            *mems[MAX_COUNT];
};

void mempool_basic_test();
void mempool_guard_test();

#endif  // INCLUDE_YWMEMPOOL_H_
