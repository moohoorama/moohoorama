/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWUTIL_H_
#define INCLUDE_YWUTIL_H_

#include <ywcommon.h>

void ywuGlobalInit();
void dump_stack();
void printVar(const char * name, uint32_t size, char * buf);
void printHex(uint32_t size, char * buf);

inline uint32_t simple_hash(size_t len, char * body) {
    uint32_t h = 5381;

    for (size_t i = 0; i < len; i++) {
        h = ((h << 5) + h) ^ body[i];
    }

    return h;
}

#endif  // INCLUDE_YWUTIL_H_
