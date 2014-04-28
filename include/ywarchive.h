/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWARCHIVE_H_
#define INCLUDE_YWARCHIVE_H_

#include <ywcommon.h>

class ywar_print {
 public:
    template<typename T>
    void dump(T val, const char * title = NULL) {
        printf("%24s :", title);
        val.dump();
        printf("\n");
    }
    void finalize() {
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    }
};

#endif  // INCLUDE_YWARCHIVE_H_
