/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSTACK_H_
#define INCLUDE_YWSTACK_H_

#include <ywcommon.h>
#include <ywworker.h>
#include <ywspinlock.h>
#include <ywpool.h>
#include <string.h>
#include <stdlib.h>

template <typename T, T NIL, int32_t NIL_IDX = -1, int32_t MAX_COUNT = 16>
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

    T pop() {
        if (count) {
            return slot[--count];
        }

        return NIL;
    }

    int32_t find(T key) {
        int32_t i = count;

        while (i--) {
            if (slot[i] == key) {
                return i;
            }
        }

        return NIL_IDX;
    }

    int32_t get_count() {
        return count;
    }

    T get(int32_t n) {
        if ((0 <= n) && (n < count)) {
            return slot[n];
        }
        return NIL;
    }

    void clear() {
        count = 0;
    }

 private:
    int32_t count;
    T       slot[MAX_COUNT];
};

void stack_basic_test();

#endif  // INCLUDE_YWSTACK_H_
