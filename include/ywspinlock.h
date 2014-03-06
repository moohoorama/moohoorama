/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSPINLOCK_H_
#define INCLUDE_YWSPINLOCK_H_

#include <ywcommon.h>
#include <ywthread.h>

class ywSpinLock {
 public:
    static const int16_t NONE  = 0;
    static const int16_t WLOCK = -1234;
    static const uint32_t DEFAULT_TIMEOUT = 1000000;

    inline bool tryRLock() {
        int32_t prev = status;
        if (prev < 0)
            return false;
        return __sync_bool_compare_and_swap(&status, prev, prev+1);
    }

    inline bool tryWLock() {
        int32_t prev = status;
        if (prev)
            return false;
        if (__sync_bool_compare_and_swap(&status, prev, WLOCK)) {
            assert(wlock_tid == -1);
            wlock_tid = ywThreadPool::get_instance()->get_thread_id();
            assert(wlock_tid ==
                    ywThreadPool::get_instance()->get_thread_id());
            return true;
        }
        return false;
    }
    inline void release() {
        if (status < 0) {
            assert(status == WLOCK);
            assert(wlock_tid ==
                    ywThreadPool::get_instance()->get_thread_id());
            wlock_tid = -1;
            assert(__sync_bool_compare_and_swap(&status, WLOCK, NONE));
        } else {
            int32_t prev;
            do {
                prev = status;
                assert(prev > 0);
                assert(prev-1 >= 0);
            } while (!__sync_bool_compare_and_swap(&status, prev, prev-1));
        }
    }

    inline bool RLock(uint32_t timeout = DEFAULT_TIMEOUT) {
        uint32_t i;

        for (i = 0; i < timeout; ++i) {
            if (tryRLock()) {
                return true;
            }
        }
        miss_count++;

        return false;
    }

    bool WLock(uint32_t timeout = DEFAULT_TIMEOUT) {
        uint32_t i;

        for (i = 0; i < timeout; ++i) {
            if (tryWLock()) {
                return true;
            }
        }
        miss_count++;

        return false;
    }

    bool hasWLock() {
        return status == WLOCK;
    }
    bool hasRLock() {
        return status > 0;
    }
    bool hasNoLock() {
        return status == NONE;
    }

    ywSpinLock():status(0), miss_count(0), wlock_tid(-1) {
    }

 private:
    volatile int16_t status;
    uint16_t miss_count;
    volatile ywTID    wlock_tid;
};

class ywLockGuard {
 public:
    explicit ywLockGuard():target(NULL), locked(false) {
    }
    explicit ywLockGuard(ywSpinLock *_target):target(_target), locked(false) {
    }
    bool isEqual(ywSpinLock *_target) {
        return target == _target;
    }
    bool hasWLock() {
        return target->hasWLock();
    }
    void set(ywSpinLock *_target) {
        assert(!target);
        target = _target;
    }
    bool WLock() {
        assert(target);
        assert(!locked);
        return locked = target->WLock();
    }
    bool RLock() {
        assert(target);
        assert(!locked);
        return locked = target->RLock();
    }
    void release() {
        if (locked) {
            target->release();
        }
    }
    ~ywLockGuard() {
        release();
    }

 private:
    ywSpinLock *target;
    bool        locked;
};


#endif  // INCLUDE_YWSPINLOCK_H_
