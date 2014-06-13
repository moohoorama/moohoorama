/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <ywtypes.h>

#include <algorithm>

#define TYPE_DECL_PRIMITIVE(ID, TYPE, PRINT) \
    {ID, (sizeof(TYPE)+3)/4,                                                  \
     read_f<TYPE>, write_f<TYPE>, PRINT, comp_f<TYPE>,                        \
     constant_f<sizeof(TYPE)>, #TYPE}

#define TYPE_DECL(ID, TYPE, READ, WRITE, PRINT, COMP, GET_IMG_SIZE) \
        {ID, (sizeof(TYPE)+3)/4, READ, WRITE, PRINT, COMP, GET_IMG_SIZE, #TYPE}

/* common function*/
template<ssize_t val>
ssize_t constant_f(type_info * /*src*/) {
    return val;
}

template<typename T> void read_f(type_info *src, Byte *buf) {
    memcpy(src, buf, sizeof(T));
}

template<typename T> void write_f(type_info *src, Byte *buf) {
    memcpy(buf, src, sizeof(T));
}

template<typename T> int32_t comp_f(type_info *left, type_info *right) {
    T *l = reinterpret_cast<T*>(left);
    T *r = reinterpret_cast<T*>(right);

    return r->val - l->val;
}

/* Integer */
typedef struct {
    int32_t val;
} ywtInt;

void print_ywtint(type_info *src) {
    ywtInt *val = reinterpret_cast<ywtInt*>(src);
    printf("%" PRId32, val->val);
}

/* Long */
typedef struct {
    int64_t val;
} ywtLong;

void print_ywtlong(type_info *src) {
    ywtLong *val = reinterpret_cast<ywtLong*>(src);
    printf("%" PRId64, val->val);
}

/* ptr */
typedef struct {
    intptr_t val;
} ywtPtr;

void print_ywtptr(type_info *src) {
    ywtPtr *val = reinterpret_cast<ywtPtr*>(src);
    printf("%" PRId64, val->val);
}

/* Barray */
typedef struct {
    static const Byte LONG_LEN = 250;

    Byte     *value;
    int32_t   len;
} ywtBarray;

void read_ywtbarray(type_info *src, Byte *buf) {
    ywtBarray *val = reinterpret_cast<ywtBarray*>(src);

    if (buf[0] == ywtBarray::LONG_LEN) {
        read_f<int32_t>(reinterpret_cast<type_info*>(val->len), buf+1);
        val->value = buf + 5;
    } else {
        val->len = buf[0];
        val->value = buf + 1;
    }
}

void write_ywtbarray(type_info *src, Byte *buf) {
    ywtBarray *val = reinterpret_cast<ywtBarray*>(src);

    if (val->len >= ywtBarray::LONG_LEN) {
        buf[0] = ywtBarray::LONG_LEN;
        write_f<int32_t>(reinterpret_cast<type_info*>(val->len), buf+1);
        memcpy(buf+5, val->value, val->len);
    } else {
        buf[0] = val->len;
        memcpy(buf+1, val->value, val->len);
    }
}

void print_ywtbarray(type_info *src) {
    ywtBarray *val = reinterpret_cast<ywtBarray*>(src);

    printHex(val->len, val->value, false/*info*/);
    printf(" ");
}

int32_t comp_ywtbarray(type_info *left, type_info *right) {
    ywtBarray *l = reinterpret_cast<ywtBarray*>(left);
    ywtBarray *r = reinterpret_cast<ywtBarray*>(right);
    uint32_t   min_len;
    int32_t    ret;

    min_len = std::min(l->len, r->len);

    if (0 != (ret = memcmp(l->value, r->value, min_len))) {
        return ret;
    }

    return r->len - l->len;
}

ssize_t get_img_size_ywtbarray(type_info *src) {
    ywtBarray *val = reinterpret_cast<ywtBarray*>(src);

    if (val->len >= ywtBarray::LONG_LEN) {
        return val->len + 5;
    }

    return val->len + 1;
}

const TypeModule type_modules[] = {
    TYPE_DECL_PRIMITIVE(0, ywtInt,  print_ywtint),
    TYPE_DECL_PRIMITIVE(1, ywtLong, print_ywtlong),
    TYPE_DECL_PRIMITIVE(2, ywtPtr,  print_ywtptr),
    TYPE_DECL(3, ywtBarray, read_ywtbarray, write_ywtbarray,
              print_ywtbarray, comp_ywtbarray,
              get_img_size_ywtbarray)
};

