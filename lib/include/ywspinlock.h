/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSPINLOCK_H_
#define INCLUDE_YWSPINLOCK_H_

#include <ywcommon.h>

class ywSpinLock {
 public:
    static const int16_t NONE  = 0;
    static const int16_t WLOCK = -1;
    static const uint32_t DEFAULT_TIMEOUT = 1000000;

    bool tryRLock();
    bool tryWLock();
    void release();

    bool RLock(uint32_t timeout = DEFAULT_TIMEOUT);
    bool WLock(uint32_t timeout = DEFAULT_TIMEOUT);

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
