/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWLOGTYPE_H_
#define INCLUDE_YWLOGTYPE_H_

#include <ywcommon.h>
#include <ywchunkmgr.h>

class ywAllocChunk {
    explicit ywAllocChunk(int32_t _map_idx, ywCnkID _id):
        map_idx(_map_idx), id(_id) {
    }
    void write(Byte *src) {
        uint32_t vid = id.get_int();
        memcpy(src, &map_idx, map_idx);
        memcpy(src, &map_idx, map_idx);
    }
    int32_t   get_size() {
        return size;
    }

 private:
    int32_t map_idx;
    ywCnkID id;
};

class ywLogWrapper {
 public:
    explicit ywLogWrapper() {
        size = 0;
    }
    explicit ywLogWrapper(int32_t _size, Byte *_val):size(_size), val(_val) {
    }
    void write(Byte *src) {
        memcpy(src, &val, size);
    }
    int32_t   get_size() {
        return size;
    }
    void     print() {
        printf("%8d 0x%" PRIxPTR " ", size,
               reinterpret_cast<intptr_t>(val));
    }

    int32_t size;
    Byte    *val;
};



#endif  // INCLUDE_YWLOGTYPE_H_
