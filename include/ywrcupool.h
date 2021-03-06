/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWRCUPOOL_H_
#define INCLUDE_YWRCUPOOL_H_

/*******************************************
 * RCU Algorithm based Mempool
 *******************************************/

#include <ywcommon.h>
#include <ywrcuref.h>
#include <ywmempool.h>
#include <ywstack.h>

template<typename T, size_t CHUNK_SIZE = 64*KB>
class ywRcuPool {
 public:
    ywRcuPool() {
        static_assert(sizeof(T) >= sizeof(ywDList), "Too small datatype");
    }

    ywrcu_slot *fix() {
        return rcu_ref.fix();
    }
    void        unfix(ywrcu_slot *slot) {
        rcu_ref.unfix(slot);
    }

    T * alloc() {
        T * ret = rcu_ref.get_reusable_item<T>();
        if (!ret) {
            ret = mem_pool.alloc();
        }
        return ret;
    }

    void free(T * ptr) {
        rcu_ref.regist_free_obj<T>(ptr);
    }

    void report() {
        printf("MEMPool: ChunkCount : %d\n", mem_pool.get_chunk_count());
        printf("         FreeCount  : %" PRIdPTR "\n",
               mem_pool.get_free_count());
        printf("         All_free   : %d\n", mem_pool.all_free());
        printf("RCURef:  freeCount  : %" PRIdPTR "\n",
               rcu_ref.get_free_count());
    }

    bool all_free() {
        return mem_pool.all_free();
    }

    void aging() {
        rcu_ref.aging();
    }

    void reclaim_to_pool() {
        T * ret;

        ret = rcu_ref.get_reusable_item<T>();
        while (ret) {
            mem_pool.free_mem(ret);
            ret = rcu_ref.get_reusable_item<T>();
        }
    }

 private:
    ywMemPool<T, CHUNK_SIZE>  mem_pool;
    ywRcuRef                  rcu_ref;
};

template<typename T>
class ywRcuPoolGuard {
 public:
    explicit ywRcuPoolGuard(ywRcuPool<T> *_target):target(_target) {
        slot = target->fix();
    }
    ~ywRcuPoolGuard() {
        rollback();
        target->unfix(slot);
    }

    T *alloc() {
        T *ptr = target->alloc();
        if (ptr) {
            assert(alloc_stack.find(ptr) == -1);
            assert(free_stack.find(ptr) == -1);
            if (alloc_stack.push(ptr)) {
                return ptr;
            }
            target->free(ptr);
        }
        return NULL;
    }

    bool free(T *ptr) {
        assert(alloc_stack.find(ptr) == -1);
        assert(free_stack.find(ptr) == -1);
        return free_stack.push(ptr);
    }

    void commit() {
        T *ptr;

        alloc_stack.clear();
        while (free_stack.pop(&ptr)) {
            target->free(ptr);
        }
    }

    void rollback() {
        T *ptr;

        while (alloc_stack.pop(&ptr)) {
            target->free(ptr);
        }
        free_stack.clear();
    }

 private:
    ywRcuPool<T>  *target;
    ywStack<T*>   alloc_stack;
    ywStack<T*>   free_stack;
    ywrcu_slot    *slot;
};

extern void rcu_pool_test();

#endif  // INCLUDE_YWRCUPOOL_H_
