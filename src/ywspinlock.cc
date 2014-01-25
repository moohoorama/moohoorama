/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywspinlock.h>
#include <assert.h>

bool ywSpinLock::tryRLock() {
    int32_t prev = status;
    if (prev < 0)
        return false;
    return __sync_bool_compare_and_swap(&status, prev, prev+1);
}

bool ywSpinLock::tryWLock() {
    int32_t prev = status;
    if (prev)
        return false;
    return __sync_bool_compare_and_swap(&status, prev, WLOCK);
}

void ywSpinLock::release() {
    if (status < 0) {
        assert(__sync_bool_compare_and_swap(&status, WLOCK, NONE));
    } else {
        __sync_fetch_and_sub(&status, 1);
    }
}

bool ywSpinLock::RLock(uint32_t timeout) {
    uint32_t i;

    for (i = 0; i < timeout; ++i) {
        if (tryRLock()) {
            return true;
        }
    }
    miss_count++;

    return false;
}

bool ywSpinLock::WLock(uint32_t timeout) {
    uint32_t i;

    for (i = 0; i < timeout; ++i) {
        if (tryWLock()) {
            return true;
        }
    }
    miss_count++;

    return false;
}


