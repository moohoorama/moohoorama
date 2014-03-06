/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWTHREAD_H_
#define INCLUDE_YWTHREAD_H_

#include <ywcommon.h>

typedef int32_t ywTID;
typedef void (*ywTaskFunc)(void *arg);

class ywThreadPool{
 public:
    static const int32_t     MAX_QUEUE_SIZE   = 64;
    static const int32_t     SLEEP_INTERVAL   = 1000;
    static int32_t           NULL_ARG_PTR;

 private:
    ywThreadPool():
        g_tid(0),
        thread_count(0),
        block(true),
        done(false),
        queue_begin(0),
        queue_end(0) {
        thread_count = 0;
        init();
    }
    explicit ywThreadPool(int32_t _thread_count):
        g_tid(0),
        thread_count(_thread_count),
        block(true),
        done(false),
        queue_begin(0),
        queue_end(0) {
        init();
    }
    ~ywThreadPool() {
        done = true;
    }

    void init();
    static void *work(void *arg_ptr);

 public:
    inline static ywThreadPool *get_instance() {
        return &gInstance;
    }

    static inline void    null_func(void *arg) {}

    static inline int32_t get_processor_count() {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }

    static inline ywTID get_thread_id() {
        static __thread ywTID tid = -1;
        if (_UNLIKELY(tid == -1)) {
            tid = __sync_fetch_and_add(
                &get_instance()->g_tid, 1);
        }
        return tid;
    }

    inline ywTID get_max_thread_id() {
        return g_tid;
    }

    inline bool add_task(ywTaskFunc func, void *arg) {
        if (block) {
            return false;
        }

        int32_t prev;
        int32_t next;
        do {
            prev = queue_end;
            next = (prev + 1) % MAX_QUEUE_SIZE;
            if (next == queue_begin) {
                return false;
            }
        } while ( !__sync_bool_compare_and_swap(
                &queue_end, prev, next));

        while (!__sync_bool_compare_and_swap(
                &funcs[prev], (ywTaskFunc)null_func, func)) {
        }
        while (!__sync_bool_compare_and_swap(
                &(args[prev]), &NULL_ARG_PTR, arg)) {
        }

        return true;
    }

    inline int get_running_thread_count() {
        int32_t i;
        int32_t ret = 0;

        __sync_synchronize();
        for (i = 0; i < thread_count; ++i) {
            ret += (running[i] == true);
        }

        return ret;
    }

    inline void block_add_task(bool _new) {
        block = _new;
    }

    inline void wait_to_idle() {
        block_add_task(true);
        while (!is_idle()) {
            usleep(SLEEP_INTERVAL);
        }
        assert(queue_end == queue_begin);

        block_add_task(false);
    }
    inline bool is_idle() {
        if (queue_end == queue_begin) {
            if (get_running_thread_count() == 0) {
                return true;
            }
        }
        return false;
    }


 private:
    inline bool get_task(ywTaskFunc *func, void **arg) {
        int32_t    prev;
        int32_t    next;

        do {
            prev = queue_begin;
            *arg = args[prev];
            if (*arg == &NULL_ARG_PTR) {
                return false;
            }
            *func = funcs[prev];
            next = (prev + 1) % MAX_QUEUE_SIZE;
        } while (!__sync_bool_compare_and_swap(
                 &args[prev], *arg, &NULL_ARG_PTR));

        while (!__sync_bool_compare_and_swap(
                &funcs[prev], *func, (ywTaskFunc)null_func)) {
        }

        while (!__sync_bool_compare_and_swap(
                &queue_begin, prev, next)) {
        }

        return true;
    }

    ywTID         g_tid;
    int32_t       thread_count;
    bool          block;
    bool          done;
    pthread_t     pt[MAX_THREAD_COUNT];
    volatile bool running[MAX_THREAD_COUNT];

    volatile int32_t     queue_begin;
    volatile int32_t     queue_end;
    ywTaskFunc           funcs[MAX_QUEUE_SIZE];
    void                *args[MAX_QUEUE_SIZE];

    static ywThreadPool gInstance;
};

extern void threadpool_test();

#endif  // INCLUDE_YWTHREAD_H_
