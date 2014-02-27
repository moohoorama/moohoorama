/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWLOCKARRAY_H_
#define INCLUDE_YWLOCKARRAY_H_

#include <ywcommon.h>
#include <ywthread.h>
#include <ywspinlock.h>
#include <ywsll.h>

class ywLockArray {
    static const int32_t LOCK_COUNT = 65536;

    ywSpinLock lock[LOCK_COUNT];
};

extern void lock_array_test();
#endif  // INCLUDE_YWLOCKARRAY_H_
