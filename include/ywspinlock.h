/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSPINLOCK_H_
#define INCLUDE_YWSPINLOCK_H_

#include <stdint.h>

class ywSpinLock {
 public:
    static const int32_t NONE  = 0;
    static const int32_t WLOCK = -1;
    static const uint32_t DEFAULT_TIMEOUT = 1000000;

    bool tryRLock();
    bool tryWLock();
    void release();

    bool RLock(uint32_t timeout = DEFAULT_TIMEOUT);
    bool WLock(uint32_t timeout = DEFAULT_TIMEOUT);


    ywSpinLock():status(0), miss_count(0) {
    }

 private:
    int32_t status;
    uint32_t miss_count;
};

#endif  // INCLUDE_YWSPINLOCK_H_
