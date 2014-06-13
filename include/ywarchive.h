/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWARCHIVE_H_
#define INCLUDE_YWARCHIVE_H_

#include <ywcommon.h>
#include <ywtypes.h>


#define AR_DUMP(FUNC, VAR, NAME)  if (!ar->FUNC(&(VAR), NAME)) return false;

class ywar_print {
 public:
    void initialize() {
    }
    template<typename T>
    bool dump(T *val, const char * title = NULL) {
        printf("%24s :", title);
        val->print();
        printf("\n");
        return true;
    }

    bool dump_int(uint64_t *val, const char * title = NULL) {
        printf("%24s : %" PRId64 "\n", title, *val);

        return true;
    }

    bool dump_int(int64_t *val, const char * title = NULL) {
        printf("%24s : %" PRIu64 "\n", title, *val);

        return true;
    }


    template<typename T>
    bool dump_int(T *val, const char * title = NULL) {
        static_assert(sizeof(T) <= 4, "invalid type");

        printf("%24s : %" PRId32 "\n", title, *val);
        return true;
    }

    void finalize() {
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    }
};

class ywar_deserialize {
 public:
    explicit ywar_deserialize(ssize_t size, Byte *buffer):
        _size(size), _buffer(buffer), _offset(0) {
    }

    void initialize() {
    }

    template<typename T>
    bool dump(T *val, const char * title = NULL) {
        val->read(&_buffer[_offset]);
        if (_offset + val->get_size() > _size) {
            return false;
        }
        _offset += val->get_size();
        return true;
    }

    template<typename T>
    bool dump_int(T *val, const char * title = NULL) {
        static_assert(sizeof(T) <= 8, "invalid type");

        if (_offset + sizeof(T) > _size) {
            return false;
        }
        memcpy(val, &_buffer[_offset], sizeof(T));
        _offset += sizeof(T);
        return true;
    }

    void finalize() {
    }

 private:
    ssize_t  _size;
    Byte    *_buffer;
    ssize_t  _offset;
};

class ywar_serialize {
 public:
    explicit ywar_serialize(size_t size, Byte *buffer):
        _size(size), _buffer(buffer), _offset(0) {
    }

    void initialize() {
    }

    template<typename T>
    bool dump(T *val, const char * title = NULL) {
        if (_offset + val->get_size() > _size) {
            return false;
        }
        val->write(&_buffer[_offset]);
        _offset += val->get_size();
        return true;
    }

    template<typename T>
    bool dump_int(T *val, const char * title = NULL) {
        static_assert(sizeof(T) <= 8, "invalid type");

        if (_offset + sizeof(T) > _size) {
            return false;
        }
        memcpy(&_buffer[_offset], val, sizeof(T));
        _offset += sizeof(T);
        return true;
    }

    void finalize() {
    }

 private:
    ssize_t  _size;
    Byte    *_buffer;
    ssize_t  _offset;
};

#endif  // INCLUDE_YWARCHIVE_H_
