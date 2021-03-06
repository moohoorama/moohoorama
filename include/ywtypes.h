/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWTYPES_H_
#define INCLUDE_YWTYPES_H_

#include <ywcommon.h>
#include <ywutil.h>

class ywInt {
 public:
    explicit ywInt() {
        val = 0;
    }
    explicit ywInt(Byte *src) {
        read(src);
    }
    explicit ywInt(int32_t src) {
        val = src;
    }
    void read(Byte *src) {
        memcpy(&val, src, sizeof(val));
    }
    void write(Byte *src) {
        memcpy(src, &val, sizeof(val));
    }
    int32_t  compare(ywInt right) {
        return compare(&right);
    }
    int32_t  compare(ywInt *right) {
        return right->val - val;
    }
    int32_t   get_size() {
        return sizeof(val);
    }
    void     print() {
        printf("%8d ", val);
    }

    int32_t val;
};

class ywLong {
 public:
    explicit ywLong() {
        val = 0;
    }
    explicit ywLong(Byte *src) {
        read(src);
    }
    explicit ywLong(int32_t src) {
        val = src;
    }
    void read(Byte *src) {
        memcpy(&val, src, sizeof(val));
    }
    void write(Byte *src) {
        memcpy(src, &val, sizeof(val));
    }
    int32_t  compare(ywLong right) {
        return compare(&right);
    }
    int32_t  compare(ywLong *right) {
        return right->val - val;
    }
    int32_t   get_size() {
        return sizeof(val);
    }
    void     print() {
        printf("%8" PRId64 " ", val);
    }

    int64_t val;
};

class ywPtr {
 public:
    explicit ywPtr() {
        ptr = NULL;
    }
    explicit ywPtr(void *_ptr):ptr(_ptr) {
    }
    explicit ywPtr(Byte *_ptr) {
        read(_ptr);
    }
    void read(Byte *src) {
        memcpy(&ptr, src, sizeof(ptr));
    }
    void write(Byte *src) {
        memcpy(src, &ptr, sizeof(ptr));
    }
    int32_t  compare(ywPtr right) {
        return compare(&right);
    }
    int32_t  compare(ywPtr *right) {
        return right->get_intptr() - get_intptr();
    }
    intptr_t get_intptr() {
        return reinterpret_cast<intptr_t>(ptr);
    }
    void *get() {
        return ptr;
    }
    int32_t   get_size() {
        return sizeof(ptr);
    }
    void     print() {
        printf("0x%08" PRIxPTR " ", get_intptr());
    }

 private:
    void * ptr;
};

#endif  // INCLUDE_YWTYPES_H_
