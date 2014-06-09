/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWARCHIVE_H_
#define INCLUDE_YWARCHIVE_H_

#include <ywcommon.h>

class ywar_print {
 public:
    void initialize() {
    }
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

class ywar_bytestream {
 public:
    explicit ywar_bytestream(Byte *buffer):_buffer(buffer), _offset(0) {
    }

    void initialize() {
    }

    template<typename T>
    void dump(T val, const char * title = NULL) {
        val.write(&_buffer[_offset]);
        _offset += val.get_size();
    }
    void finalize() {
    }

 private:
    Byte    *_buffer;
    ssize_t  _offset;
};


#endif  // INCLUDE_YWARCHIVE_H_
