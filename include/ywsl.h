/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#ifndef INCLUDE_YWSL_H_
#define INCLUDE_YWSL_H_

#include <stdlib.h>
#include <stdint.h>
#include <ywspinlock.h>

typedef struct _ywNode {
    _ywNode():prev(this), next(this), data(NULL) {
    }

    struct _ywNode     *prev;
    struct _ywNode     *next;
    ywSpinLock          lock;
    void               *data;
} ywNode;

class ywSyncList {
 public:
    static const int32_t  NONE  = 0;
    static const int32_t  WLOCK = -1;
    static const uint32_t DEFAULT_TIMEOUT = 1000000;

    bool add(ywNode *node);
    bool remove(ywNode *node);

    ywNode *begin() {
        return head.next;
    }
    ywNode *end() {
        return &head;
    }
    void print();

    ywSyncList():count(0) {
    }

 private:
    ywNode   head;
    int64_t  count;
};

#endif  // INCLUDE_YWSL_H_

