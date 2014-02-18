/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWTHREAD_H_
#define INCLUDE_YWTHREAD_H_

#include <ywcommon.h>

typedef int32_t ywTID;
typedef void (*ywTaskFunc)(void *arg);

class ywThreadPool{
 public:
    static const int32_t MAX_QUEUE_SIZE   = 64;
    static const int32_t SLEEP_INTERVAL   = 100;

 private:
    ywThreadPool():
        g_tid(0),
        block_add_task(true),
        done(false),
        queue_begin(0),
        queue_end(0) {
        thread_count = get_processor_count();
        assert(thread_count <= MAX_PROCESSOR_COUNT);
        init();
    }
    explicit ywThreadPool(int32_t _thread_count):
        g_tid(0),
        thread_count(_thread_count),
        block_add_task(true),
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

    static inline int32_t get_processor_count() {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }

    inline ywTID get_thread_id() {
        static __thread ywTID tid = -1;
        if (_UNLIKELY(tid == -1)) {
            tid = __sync_fetch_and_add(&g_tid, 1);
        }
        return tid;
    }

    inline ywTID get_max_thread_id() {
        return g_tid;
    }

    inline bool add_task(ywTaskFunc func, void *arg) {
        if (block_add_task) {
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
                &funcs[prev], NULL, func)) {
        }

        assert(__sync_bool_compare_and_swap(
                &args[prev], NULL, arg));

        return true;
    }

    inline int get_running_thread_count() {
        int32_t i;
        int32_t ret = 0;

        for (i = 0; i < thread_count; ++i) {
            ret += (running[i] == true);
        }

        return ret;
    }

    inline void set_block_add_task(bool _new) {
        block_add_task = _new;
    }

    inline void wait_to_idle() {
        set_block_add_task(true);
        while (queue_end != queue_begin) {
            usleep(SLEEP_INTERVAL);
        }

        while (get_running_thread_count()) {
            usleep(SLEEP_INTERVAL);
        }
        set_block_add_task(false);
    }

 private:
    inline bool get_task(ywTaskFunc *func, void **arg) {
        int32_t    prev;
        int32_t    next;

        do {
            prev = queue_begin;
            *func = funcs[prev];
            if (*func == NULL) {
                return false;
            }
            *arg  = args[prev];
            next = (prev + 1) % MAX_QUEUE_SIZE;
        } while ( !__sync_bool_compare_and_swap(
                &funcs[prev], *func, NULL));

        assert(__sync_bool_compare_and_swap(
                &args[prev], *arg, NULL));
        assert(__sync_bool_compare_and_swap(
                &queue_begin, prev, next));

        return true;
    }

    ywTID       g_tid;
    int32_t     thread_count;

    bool        block_add_task;
    bool        done;

    pthread_t   pt[MAX_PROCESSOR_COUNT];
    bool        running[MAX_PROCESSOR_COUNT];

    int32_t     queue_begin;
    int32_t     queue_end;
    ywTaskFunc  funcs[MAX_QUEUE_SIZE];
    void       *args[MAX_QUEUE_SIZE];

    static ywThreadPool gInstance;
};

extern void threadpool_test();

#endif  // INCLUDE_YWTHREAD_H_
