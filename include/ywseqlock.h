/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#ifndef INCLUDE_YWSEQLOCK_H_
#define INCLUDE_YWSEQLOCK_H_

#include <ywcommon.h>

class ywSeqLockGuard {
    static const uint32_t LOCK_MASK  = 0x1;
    typedef enum {
        NONE,
        INIT,
        LOCKED,
        READ
    } guardStatus;

 public:
    ywSeqLockGuard():lock_seq(NULL), status(NONE) {
    }
    explicit ywSeqLockGuard(volatile uint32_t *_lock_seq):
        lock_seq(_lock_seq), status(INIT) {
    }
    ~ywSeqLockGuard() {
        release();
    }

    void set(volatile uint32_t *_lock_seq) {
        lock_seq = _lock_seq;
        status = INIT;
    }

    void read_begin() {
        assert(status == INIT);

        status   = READ;
        read_seq = *lock_seq;
        __sync_synchronize();
    }

    bool read_end() {
        assert(status == READ);
        __sync_synchronize();
        status = INIT;
        return ((read_seq & LOCK_MASK) == 0) && (read_seq == *lock_seq);
    }

    bool lock() {
        uint32_t prev = *lock_seq;

        assert(status == INIT);

        if (prev & LOCK_MASK) return false;
        if (__sync_bool_compare_and_swap(lock_seq, prev, prev | LOCK_MASK)) {
            status = LOCKED;
            return true;
        }
        return false;
    }
    void release() {
        if (status == LOCKED) {
            uint32_t prev = *lock_seq;
            status = INIT;
            assert(__sync_bool_compare_and_swap(lock_seq, prev, prev + 1));
        } else {
            if (status == READ) {
                status = INIT;
            }
        }
    }

 private:
    volatile uint32_t      *lock_seq;
    uint32_t                read_seq;
    guardStatus             status;
};
#endif  // INCLUDE_YWSEQLOCK_H_
