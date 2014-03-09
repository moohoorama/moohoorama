/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSPINLOCK_H_
#define INCLUDE_YWSPINLOCK_H_

#include <ywcommon.h>
#include <ywthread.h>

//   #define __CHECK_WLOCK_TID__

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
#ifdef __CHECK_WLOCK_TID__
            assert(wlock_tid == -1);
            wlock_tid = ywThreadPool::get_instance()->get_thread_id();
#endif
            return true;
        }
        return false;
    }
    inline void release() {
        if (status < 0) {
#ifdef __CHECK_WLOCK_TID__
            assert(status == WLOCK);
            assert(wlock_tid ==
                    ywThreadPool::get_instance()->get_thread_id());
            wlock_tid = -1;
#endif
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

    ywSpinLock():status(0), miss_count(0)
#ifdef __CHECK_WLOCK_TID__
                 , wlock_tid(-1)
#endif
    {
    }

 private:
    volatile int16_t status;
    uint16_t miss_count;
#ifdef __CHECK_WLOCK_TID__
    volatile ywTID    wlock_tid;
#endif
};

template<int32_t GUARD_SIZE = 16>
class ywLockGuard {
 public:
    ywLockGuard():rlock_idx(0), wlock_idx(0) {
    }

    bool WLock(ywSpinLock *lock, int32_t line) {
        if (wlock_idx >= GUARD_SIZE) return false;

        if (wlock_idx > 0) {
            int32_t i = wlock_idx;
            while (i--) {
                if (wlock[i] == lock) {
                    return true;
                }
            }
        }

        if (lock->WLock()) {
            wlock_line[ wlock_idx ] = line;
            wlock[ wlock_idx++] = lock;
            return true;
        }
        return false;
    }

    bool RLock(ywSpinLock *lock) {
        if (rlock_idx >= GUARD_SIZE) return false;

        if (wlock_idx > 0) {
            int32_t i = wlock_idx;
            while (i--) {
                if (wlock[i] == lock) {
                    return true;
                }
            }
        }

        if (rlock_idx > 0) {
            int32_t i = rlock_idx;
            while (i--) {
                if (rlock[i] == lock) {
                    return true;
                }
            }
        }


        if (lock->RLock()) {
            rlock[ rlock_idx++] = lock;
            return true;
        }
        return false;
    }

    void release() {
        if (rlock_idx > 0) {
            int32_t i = rlock_idx;
            while (i--) {
                rlock[ i ]->release();
            }
            rlock_idx = 0;
        }
        if (wlock_idx > 0) {
            int32_t i = wlock_idx;
            while (i--) {
                wlock[ i ]->release();
            }
            wlock_idx = 0;
        }
    }
    ~ywLockGuard() {
        release();
    }

 private:
    ywSpinLock *rlock[GUARD_SIZE];
    int32_t     rlock_idx;

    ywSpinLock *wlock[GUARD_SIZE];
    int32_t     wlock_line[GUARD_SIZE];
    int32_t     wlock_idx;
};


#endif  // INCLUDE_YWSPINLOCK_H_
