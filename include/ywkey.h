/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWKEY_H_
#define INCLUDE_YWKEY_H_

#include <ywcommon.h>
#include <ywutil.h>

template<typename KEY, typename VAL>
class ywKey {
 public:
    explicit ywKey(Byte *src):key(src), val(src + key.get_size()) {
    }
    explicit ywKey(KEY _key, VAL _val):key(_key), val(_val) {
    }
    void read(Byte *src) {
        key.read(src);
        val.read(src + key.get_size());
    }
    void write(Byte *src) {
        key.write(src);
        val.write(src + key.get_size());
    }
    int32_t   comp(Byte *_right) {
        return key.comp(_right);
    }
    int32_t   get_size() {
        return key.get_size() + val.get_size();
    }

    KEY       key;
    VAL       val;
};

extern void key_test();

#endif  // INCLUDE_YWKEY_H_
