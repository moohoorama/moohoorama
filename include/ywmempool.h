/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWMEMPOOL_H_
#define INCLUDE_YWMEMPOOL_H_

#include <ywcommon.h>
#include <ywworker.h>
#include <ywspinlock.h>
#include <ywpool.h>
#include <string.h>
#include <stdlib.h>

#include <typeinfo>

template<typename T, size_t CHUNK_SIZE =  64*KB>
class ywMemPool {
    static const size_t  UNIT = sizeof(T);
    static const size_t  CHUNK_MAX_COUNT =   2*GB / CHUNK_SIZE;

 public:
    explicit ywMemPool() {
        init();
    }

    ~ywMemPool() {
        free_all();
    }

    bool all_free() {
        size_t per_count = CHUNK_SIZE/UNIT;
        return (get_free_count() == get_chunk_count()*per_count);
    }

    inline T *  alloc() {
        T * ret;

        while (true) {
            ret = pool.pop();
            if (ret) return ret;
            if (!alloc_chunk()) {
                return NULL;
            }
        }
    }

    inline void free_mem(T * mem) {
        pool.push(mem);
    }

    size_t get_size() {
        return chunk_idx * CHUNK_SIZE;
    }

    int32_t get_chunk_count() {
        return chunk_idx;
    }

    size_t get_free_count() {
        return pool.get_pooling_count();
    }

    void report() {
        pool.print();
    }

 private:
    void init() {
        static_assert(UNIT <= CHUNK_SIZE, "Too small chunk");

        chunk_idx = 0;
    }

    void free_all() {
        while (chunk_idx--) {
            free(chunk_array[chunk_idx]);
        }
    }

    bool alloc_chunk() {
        if (chunk_idx >= CHUNK_MAX_COUNT) {
            return false;
        }
        char      * chunk = reinterpret_cast<char*>(malloc(CHUNK_SIZE));
        size_t      i;

        if (!chunk) {
            return false;
        }

        chunk_lock.WLock();
        if (chunk_idx >= CHUNK_MAX_COUNT) {
            chunk_lock.release();
            free(chunk);
            return false;
        }
        chunk_array[ chunk_idx++ ] = chunk;
        chunk_lock.release();

        for (i = 0; i + UNIT <= CHUNK_SIZE; i += UNIT) {
            free_mem(reinterpret_cast<T*>(chunk+i));
        }
        return true;
    }

    char      *chunk_array[CHUNK_MAX_COUNT];
    size_t     chunk_idx;
    ywPool<T>  pool;
    ywSpinLock chunk_lock;
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
