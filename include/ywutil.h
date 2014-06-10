/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWUTIL_H_
#define INCLUDE_YWUTIL_H_

#include <ywcommon.h>
#include <sys/time.h>
#include <sys/resource.h>

void ywuGlobalInit();
void dump_stack();
void printVar(const char * name, uint32_t size, char * buf);
void printHex(uint32_t size, char * buf, bool info = true);
void printHex(uint32_t size, Byte * buf, bool info = true);

template<typename T>
T align(T src, T unit) {
    assert((unit & ~unit) == 0);
    return (src + (unit-1)) & ~(unit-1);
}

inline uint32_t simple_hash(size_t len, char * body) {
    uint32_t h = 5381;

    for (size_t i = 0; i < len; i++) {
        h = ((h << 5) + h) ^ body[i];
    }

    return h;
}

template<typename T>
T load_consume(T const* addr) {
      // hardware fence is implicit on x86
    T v = *const_cast<T const volatile*>(addr);
    __sync_synchronize();
    return v;
}

// store with 'release' memory ordering
template<typename T>
void store_release(T* addr, T v) {
    // hardware fence is implicit on x86
    __sync_synchronize();
    *const_cast<T volatile*>(addr) = v;
}

inline size_t get_stack_size() {
    struct rlimit limit;

    getrlimit(RLIMIT_STACK, &limit);
    return limit.rlim_cur;
}

__attribute__((noinline)) void *get_pc();

#endif  // INCLUDE_YWUTIL_H_
