/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWBARRAY_H_
#define INCLUDE_YWBARRAY_H_

#include <ywcommon.h>
#include <ywutil.h>

#include <algorithm>

class ywBarray {
    static const Byte LONG_LEN = 250;

 public:
    explicit ywBarray(Byte *src) {
        read(src);
    }
    explicit ywBarray(uint32_t _len, Byte *_value):len(_len), value(_value) {
        if (len < LONG_LEN) {
            size = len + 1;
        } else {
            size = len + 5;
        }
    }
    void read(Byte *src) {
        if (src[0] == LONG_LEN) {
            len =
                (src[1] << 0) +
                (src[2] << 8) +
                (src[3] << 16) +
                (src[4] << 24);
            value = src + 5;
            size = 5 + len;
        } else {
            len = src[0];
            value = src + 1;
            size = 1 + len;
        }
    }
    void write(Byte *src) {
        if (len < LONG_LEN) {
            src[0] = len;
            memcpy(src+1, value, len);
        } else {
            src[0] = LONG_LEN;
            src[1] = ((len>>0) & 0xFF);
            src[2] = ((len>>8) & 0xFF);
            src[3] = ((len>>16) & 0xFF);
            src[4] = ((len>>24) & 0xFF);
            memcpy(src+5, value, len);
        }
    }
    int32_t   comp(Byte *_right) {
        ywBarray right(_right);
        uint32_t min_len;
        int32_t  ret;

        min_len = std::min(len, right.len);

        if (0 != (ret = memcmp(value, right.value, min_len))) {
            return ret;
        }

        return len < right.len;
    }
    int32_t   get_size() {
        return size;
    }

    uint32_t  len;
    Byte     *value;
    uint32_t  size;
};

extern void barray_test();

#endif  // INCLUDE_YWBARRAY_H_
