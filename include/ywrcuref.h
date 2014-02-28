/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWRCUREF_H_
#define INCLUDE_YWRCUREF_H_

#include <ywcommon.h>
#include <ywthread.h>
#include <ywspinlock.h>
#include <ywsll.h>

typedef uint32_t ref_slot;

template<int32_t SLOT_COUNT = 1024>
class ywRcuRef {
    static const int32_t REF_COUNT  = MAX_THREAD_COUNT;

 public:
    ywRcuRef():slot(NULL) {
    }
    ~ywRcuRef() {
        if (slot) {
            dealloc_slot();
        }
    }

    void fix(void *ptr) {
        int32_t idx = get_slot_idx(ptr);
        slot[idx] = global_refs[idx];
    }
    void unfix(void *ptr) {
        int32_t idx = get_slot_idx(ptr);
        slot[idx] = 0;
    }

    static ywRcuRef *getInstance() {
        ywTID tid = ywThreadPool::get_thread_id();

        return refs[tid];
    }

 private:
    int32_t get_slot_idx(void *ptr) {
        return simple_hash(sizeof(void*), &ptr);
    }
    ref_slot slot[SLOT_COUNT];

    static ywRcuRef refs[REF_COUNT];
    static ywRcuRef global_refs;
};

#endif  // INCLUDE_YWRCUREF_H_
