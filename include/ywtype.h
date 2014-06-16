/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWTYPE_H_
#define INCLUDE_YWTYPE_H_

#include <ywcommon.h>
#include <ywutil.h>

typedef uint32_t type_info;
typedef int32_t (*compare_func)(type_info *left, type_info *right,
                                void *l_meta, void *r_meta);
typedef void    (*read_func)(type_info *src, Byte *buf, void *meta);
typedef void    (*write_func)(type_info *src, Byte *buf, void *meta);
typedef void    (*print_func)(type_info *src, void *meta);
typedef ssize_t (*get_size_func)(type_info *src, void *meta);

const uint32_t TYPE_FLAG_FIX_SIZE = 0x00000000;
const uint32_t TYPE_FLAG_VAR_SIZE = 0x00000001;

const uint32_t TYPE_FLAG_BASIC = TYPE_FLAG_FIX_SIZE;

typedef struct TestModule {
    const int32_t            id;
    const ssize_t            info_size;
    const uint32_t           flag;
    const read_func          read;
    const write_func         write;
    const print_func         print;
    const compare_func       compare;
    const get_size_func      get_size;
    const char              *name;
} TypeModule;

extern const TypeModule type_modules[];

typedef struct {
    static const int32_t ID = 1;

    int32_t val;
} ywtInt;

typedef struct {
    static const int32_t ID = 2;

    int64_t val;
} ywtLong;

typedef struct {
    static const int32_t ID = 3;

    intptr_t val;
} ywtPtr;

typedef struct {
    static const int32_t ID = 4;
    static const Byte    LONG_LEN = 250;

    Byte     *value;
    int32_t   len;
} ywtBarray;



class ywt_archive {
    virtual void initialize() = 0;
    virtual bool dump(TestModule *module, type_info *val,
                     const char * title, void * meta) = 0;
    virtual void finalize() = 0;
};


class ywt_print : public ywt_archive {
 public:
    void initialize() { }

    bool dump(TestModule *module, type_info *val,
                     const char * title, void * meta) {
        printf("%24s :", title);
        module->print(val, meta);
        printf("\n");
        return true;
    }
    void finalize() {
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    }
};



class ywt_write : public ywt_archive {
 public:
    explicit ywt_write(size_t size, Byte *buffer):
        _size(size), _buffer(buffer), _offset(0) {
    }
    void initialize() { }

    bool dump(TestModule *module, type_info *val,
              const char * title, void * meta) {
        ssize_t img_size = module->get_size(val, meta);
        if (_offset + img_size > _size) {
            return false;
        }

        module->write(val, &_buffer[_offset], meta);
        _offset += img_size;
        return true;
    }
    void finalize() {
    }

 private:
    ssize_t  _size;
    Byte    *_buffer;
    ssize_t  _offset;
};

class ywt_read : public ywt_archive {
 public:
    explicit ywt_read(size_t size, Byte *buffer):
        _size(size), _buffer(buffer), _offset(0) {
    }
    void initialize() { }

    bool dump(TestModule *module, type_info *val,
              const char * title, void * meta) {
        module->read(val, &_buffer[_offset], meta);

        ssize_t img_size = module->get_size(val, meta);
        if (_offset + img_size > _size) {
            return false;
        }

        _offset += img_size;
        return true;
    }
    void finalize() {
    }

 private:
    ssize_t  _size;
    Byte    *_buffer;
    ssize_t  _offset;
};


extern void type_basic_test();

#endif  // INCLUDE_YWTYPE_H_
