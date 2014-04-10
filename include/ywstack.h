/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSTACK_H_
#define INCLUDE_YWSTACK_H_

#include <ywcommon.h>
#include <ywworker.h>
#include <ywspinlock.h>
#include <ywpool.h>
#include <string.h>
#include <stdlib.h>

template <typename T, int32_t MAX_COUNT = 32>
class ywStack {
 public:
    explicit ywStack(): count(0) {
    }

    ~ywStack() {
    }

    bool push(T target) {
        if (count >= MAX_COUNT) {
            return false;
        }
        slot[count++] = target;
        return true;
    }

    bool pop(T *target) {
        if (count) {
            *target = slot[--count];
            return true;
        }
        return false;
    }

    int32_t find(T key) {
        int32_t i = count;

        while (i--) {
            if (slot[i] == key) {
                return i;
            }
        }

        return -1;
    }

    int32_t get_count() {
        return count;
    }

    bool get(int32_t n, T *target) {
        if ((0 <= n) && (n < count)) {
            *target = slot[n];
            return true;
        }
        return false;
    }

    T *get_last_ptr() {
        if (count > 0) {
            return &slot[count-1];
        }
        return NULL;
    }

    void clear() {
        count = 0;
    }

 private:
    int32_t      count;
    T            slot[MAX_COUNT];
};

void stack_basic_test();

#endif  // INCLUDE_YWSTACK_H_
