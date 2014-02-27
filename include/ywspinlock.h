/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSPINLOCK_H_
#define INCLUDE_YWSPINLOCK_H_

#include <ywcommon.h>

class ywSpinLock {
 public:
    static const int16_t NONE  = 0;
    static const int16_t WLOCK = -1;
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
        return __sync_bool_compare_and_swap(&status, prev, WLOCK);
    }

    inline void release() {
        if (status < 0) {
            assert(__sync_bool_compare_and_swap(&status, WLOCK, NONE));
        } else {
            __sync_fetch_and_sub(&status, 1);
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

    ywSpinLock():status(0), miss_count(0) {
    }

 private:
    int16_t status;
    uint16_t miss_count;
};

#endif  // INCLUDE_YWSPINLOCK_H_
