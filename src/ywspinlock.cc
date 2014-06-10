/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywspinlock.h>
#include <ywtimer.h>
#include <assert.h>

void ywSpinLock::sleep(void * call_depth) {
    ywWaitEvent::u_sleep(10, call_depth);
}
