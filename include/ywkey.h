/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWKEY_H_
#define INCLUDE_YWKEY_H_

#include <ywcommon.h>
#include <ywutil.h>
#include <ywtypes.h>

template<typename KEY, typename VAL>
class ywKey : public ywTypes {
 public:
    explicit ywKey(Byte *src):key(src), val(src + key.get_size()) {
        check_inheritance<ywTypes, KEY>();
        check_inheritance<ywTypes, VAL>();
    }
    explicit ywKey(KEY _key):key(_key) {
    }
    explicit ywKey(KEY _key, VAL _val):key(_key), val(_val) {
    }
    explicit ywKey() {
    }

    void read(Byte *src) {
        key.read(src);
        val.read(src + key.get_size());
    }
    void write(Byte *src) {
        key.write(src);
        val.write(src + key.get_size());
    }
    int32_t   compare(ywKey right) {
        return compare(&right);
    }
    int32_t   compare(ywKey *right) {
        return key.compare(&(right->key));
    }
    int32_t   get_size() {
        return key.get_size() + val.get_size();
    }
    void     dump() {
        key.dump();
        val.dump();
    }

    KEY       key;
    VAL       val;
};

extern void key_test();

#endif  // INCLUDE_YWKEY_H_
