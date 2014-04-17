/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWTIMER_H_
#define INCLUDE_YWTIMER_H_

#include <ywcommon.h>
#include <sys/time.h>

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
        int32_t idx = count;
        func[idx]     = _func;
        arg[idx]      = _arg;
        interval[idx] = _interval;

        count++;

        return idx;
    }

    inline static ywTimer *get_instance() {
        return &gInstance;
    }

    static inline uint64_t get_cur_us() {
        struct timeval mytime;
        gettimeofday(&mytime, NULL);
        return (uint64_t)mytime.tv_sec * 1000 * 1000 + mytime.tv_usec;
    }

    static inline uint64_t get_cur_ms() {
        return get_cur_us() / 1000;
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

class ywPerfChecker {
    static const int32_t MILLION = 1000*1000;

 public:
    ywPerfChecker():begin(ywTimer::get_cur_us()), end(0), count(0) {
    }
    explicit ywPerfChecker(uint64_t _count):begin(ywTimer::get_cur_us()),
                end(0), count(_count) {
    }
    void start(int64_t _count = 0) {
        count = _count;
        begin = ywTimer::get_cur_us();
    }
    void finish(const char * UNIT = "TPS") {
        uint64_t elapsed;
        end = ywTimer::get_cur_us();

        elapsed = end - begin;
        if (count) {
            printf("%6.2f %s", count*MILLION*1.0f/elapsed, UNIT);
        }
        printf("%4" PRId64 " sec (%" PRId64 " msec)\n",
               elapsed / MILLION,
               (elapsed % MILLION)/1000);
    }

 private:
    uint64_t begin;
    uint64_t end;
    uint64_t count;
};

#endif  // INCLUDE_YWTIMER_H_
