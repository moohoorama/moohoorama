/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWTIMER_H_
#define INCLUDE_YWTIMER_H_

#include <ywcommon.h>

typedef void (*ywTimerFunc)(void *_arg);

class ywTimer {
    static const int32_t     MAX_FUNC_COUNT = 1024;
    static const int32_t     MIN_INTERVAL_USEC = 10;

 public:
    ywTimer() {
        pthread_attr_t  attr;

        assert(0 == pthread_attr_init(&attr) );
        assert(0 == pthread_create(&pt, &attr, run, NULL));
    }
    int32_t regist(ywTimerFunc _func, void *_arg, int32_t _interval) {
        int32_t idx = count++;
        func[idx]     = _func;
        arg[idx]      = _arg;
        interval[idx] = _interval;

        return idx;
    }

    inline static ywTimer *get_instance() {
        return &gInstance;
    }

 private:
    void         global_timer();
    static void *run(void *arg);

    pthread_t    pt;
    ywTimerFunc  func[MAX_FUNC_COUNT];
    void        *arg[MAX_FUNC_COUNT];
    int32_t      interval[MAX_FUNC_COUNT];
    int32_t      count;

    static ywTimer gInstance;
};

#endif  // INCLUDE_YWTIMER_H_
