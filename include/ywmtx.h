/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWMTX_H_
#define INCLUDE_YWMTX_H_

#include <ywcommon.h>
#include <ywstack.h>

template<int32_t MAX_COUNT =16>
class ywMtx {
 public:
    explicit ywMtx() {
    }

    ~ywMtx() {
        rollback();
    }

    bool WLock(ywSpinLock *lock) {
        if ((wlock.find(lock) == -1) &&
            (rlock.find(locK) == -1)) {
            if (lock->WLock()) {
                return wlock.push(lock);
            }
        }
        return false;
    }

    bool RLock(ywSpinLock *lock) {
        if ((wlock.find(lock) == -1) &&
            (rlock.find(locK) == -1)) {
            if (lock->RLock()) {
                return rlock.push(lock);
            }
        }
        return false;
    }



    void commit() {
        releaseLocks();
    }
    void rollback() {
        releaseLocks();
    }

 private:
    void releaseLocks() {
        ywSpinLock *lock;

        while ((lock = rlock.pop())) {
            lock.release();
        }
        while ((lock = wlock.pop())) {
            lock.release();
        }
    }
    ywStack<ywMemPool*, NULL>  allocMemPool;
    ywStack<void *, NULL>      allocPtr;
    ywStack<ywMemPool*, NULL>  dropMemPool;
    ywStack<void *, NULL>      dropPtr;

    ywrcu_slot                *rcuSlot;
    ywRcuRef                  *rcuRef;
    ywStack<void *, NULL>     *freePtr;

    ywStack<ywSpinLock*, NULL>  rlock;
    ywStack<ywSpinLock*, NULL>  wlock;
};

#endif  // INCLUDE_YWMTX_H_
