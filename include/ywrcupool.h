/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWRCUPOOL_H_
#define INCLUDE_YWRCUPOOL_H_

#include <ywcommon.h>
#include <ywrcuref.h>
#include <ywmempool.h>
#include <ywstack.h>

template<typename T>
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

 private:
    ywMemPool<T> mem_pool;
    ywRcuRef     rcu_ref;
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
            if (alloc_stack.push(ptr)) {
                return ptr;
            }
            target->free(ptr);
        }
        return NULL;
    }

    bool free(T *ptr) {
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


#endif  // INCLUDE_YWRCUPOOL_H_
