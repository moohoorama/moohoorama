/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWTIMER_H_
#define INCLUDE_YWTIMER_H_

#include <ywcommon.h>

typedef (*ywTimerFunc)();

class ywTimer {
    static const int32_t     MAX_FUNC_COUNT = 1024;

    ywTimer() {
        pthread_attr_t  attr;

        assert(0 == pthread_attr_init(&attr) );
        assert(0 == pthread_create(global_tick, NULL));
    }
    void regist(ywTimerFunc func, int32_t interval_msec);

    inline static ywThreadPool *get_instance() {
        return &gInstance;
    }

 private:
    pthread_t   pt;
    ywTimerFunc func[MAX_FUNC_COUNT];
    int32_t     interval[MAX_FUNC_COUNT];

    static void   global_tick();

    static ywTimer gInstance;
};

#endif  // INCLUDE_YWTIMER_H_
