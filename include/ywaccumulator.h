/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWACCUMULATOR_H_
#define INCLUDE_YWACCUMULATOR_H_

#include <ywcommon.h>
#include <ywthread.h>

template<typename T, bool use_atomic>
class ywAccumulator{
    T data[MAX_PROCESSOR_COUNT];

 public:
    ywAccumulator() {
        assert(ywThreadPool::get_instance()->get_max_thread_id()
               < MAX_PROCESSOR_COUNT);
        memset(data, 0, sizeof(data));
    }

    void mutate(T delta) {
        if (use_atomic == true) {
            __sync_add_and_fetch(&data[0], delta);
        } else {
            data[ywThreadPool::get_instance()->get_thread_id()] += delta;
        }
    }

    T get() {
        int i;
        T   val = 0;
        for (i = 0; i < MAX_PROCESSOR_COUNT; ++i) {
            val += data[i];
        }
        return val;
    }
};

extern void accumulator_test();
extern void atomic_stat_test();

#endif  // INCLUDE_YWACCUMULATOR_H_
