/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWMEMPOOL_H_
#define INCLUDE_YWMEMPOOL_H_

#include <ywcommon.h>
#include <ywsll.h>
#include <ywthread.h>
#include <string.h>
#include <stdlib.h>

#include <typeinfo>

template<typename T, size_t chunk_size = 32*KB>
class ywMemPool {
    static const size_t  UNIT = sizeof(T);
    static const int32_t SHARED_IDX = MAX_THREAD_COUNT;

 public:
    explicit ywMemPool() {
        init();
    }
    ~ywMemPool() {
        free_all();
    }

    inline T *  alloc() {
        int32_t     tid = ywThreadPool::get_thread_id();
        ywsllNode * free_ptr;

        while (true) {
            free_ptr = ywsll_pop(&free_list[tid]);
            if (free_ptr) {
                return reinterpret_cast<T*>(free_ptr);
            }
            if (!alloc_chunk(tid)) {
                return NULL;
            }
        }
    }
    inline void free_mem(T * mem) {
        ywsllNode * ptr = reinterpret_cast<ywsllNode*>(mem);
        int32_t     tid = ywThreadPool::get_thread_id();
        ywsll_attach(&free_list[tid], ptr);
    }

    void report() {
        printf("%s\n", typeid(this).name());
    }

 private:
    void init() {
        static_assert(UNIT <= chunk_size, "Too small chunk");
        static_assert(sizeof(ywsllNode) <= UNIT, "Too small type");

        memset(size, 0, sizeof(size));
    }

    void free_all() {
        ywsllNode * chunk_ptr;
        int32_t     i;

        for (i = 0; i < MAX_THREAD_COUNT; ++i) {
            while (size[i] > chunk_size) {
                chunk_ptr = ywsll_pop(&chunk_list[i]);
                free_mem(reinterpret_cast<T*>(chunk_ptr));
                size[i] -= chunk_size;
            }
        }
    }

    bool alloc_chunk(int tid) {
        char      * chunk = reinterpret_cast<char*>(malloc(chunk_size));
        size_t      i;

        if (!chunk) {
            return false;
        }

        for (i = sizeof(ywsllNode); i + UNIT <= chunk_size; i += UNIT) {
            free_mem(reinterpret_cast<T*>(chunk + i));
        }
        ywsll_attach(&chunk_list[tid],
                     reinterpret_cast<ywsllNode*>(chunk));
        size[tid] += chunk_size;
        return true;
    }

    size_t           size[MAX_THREAD_COUNT + 1];
    ywsllNode        chunk_list[MAX_THREAD_COUNT];
    ywsllNode        free_list[MAX_THREAD_COUNT + 1];
};

#endif  // INCLUDE_YWMEMPOOL_H_
