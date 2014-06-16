/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywtype.h>
#include <algorithm>

#define TYPE_DECL_PRIMITIVE(TYPE, PRINT) \
    {TYPE::ID, (sizeof(TYPE)+3)/4, TYPE_FLAG_BASIC,                           \
     read_f<TYPE>, write_f<TYPE>, PRINT, comp_f<TYPE>,                        \
     constant_f<sizeof(TYPE)>, #TYPE}

#define TYPE_DECL(TYPE, FLAG, READ, WRITE, PRINT, COMP, get_size) \
        {TYPE::ID, (sizeof(TYPE)+3)/4, FLAG,                      \
            READ, WRITE, PRINT, COMP, get_size, #TYPE}

void TestModule::check() {
    assert(&type_modules[id] == this);
}

/* common function*/
template<ssize_t val>
ssize_t constant_f(type_info * /*src*/, void * /*meta*/) {
    return val;
}

template<typename T> void read_f(type_info *src, Byte *buf, void * /*meta*/) {
    memcpy(src, buf, sizeof(T));
}

template<typename T> void write_f(type_info *src, Byte *buf, void * /*meta*/) {
    memcpy(buf, src, sizeof(T));
}

template<typename T> int32_t comp_f(type_info *left, type_info *right,
                                    void * /*l_meta*/, void * /*r_meta*/) {
    T *l = reinterpret_cast<T*>(left);
    T *r = reinterpret_cast<T*>(right);

    return r->val - l->val;
}

/* Integer */
void print_ywtint(type_info *src, void * /*meta*/) {
    ywtInt *val = reinterpret_cast<ywtInt*>(src);
    printf("%" PRId32, val->val);
}

/* Long */
void print_ywtlong(type_info *src, void * /*meta*/) {
    ywtLong *val = reinterpret_cast<ywtLong*>(src);
    printf("%" PRId64, val->val);
}

/* ptr */
void print_ywtptr(type_info *src, void * /*meta*/) {
    ywtPtr *val = reinterpret_cast<ywtPtr*>(src);
    printf("%" PRId64, val->val);
}

/* Barray */
void read_ywtbarray(type_info *src, Byte *buf, void * /*meta*/) {
    ywtBarray *val = reinterpret_cast<ywtBarray*>(src);

    if (buf[0] == ywtBarray::LONG_LEN) {
        read_f<int32_t>(reinterpret_cast<type_info*>(val->len), buf+1, NULL);
        val->value = buf + 5;
    } else {
        val->len = buf[0];
        val->value = buf + 1;
    }
}

void write_ywtbarray(type_info *src, Byte *buf, void * /*meta*/) {
    ywtBarray *val = reinterpret_cast<ywtBarray*>(src);

    if (val->len >= ywtBarray::LONG_LEN) {
        buf[0] = ywtBarray::LONG_LEN;
        write_f<int32_t>(reinterpret_cast<type_info*>(val->len), buf+1, NULL);
        memcpy(buf+5, val->value, val->len);
    } else {
        buf[0] = val->len;
        memcpy(buf+1, val->value, val->len);
    }
}

void print_ywtbarray(type_info *src, void * /*meta*/) {
    ywtBarray *val = reinterpret_cast<ywtBarray*>(src);

    printHex(val->len, val->value, false/*info*/);
    printf(" ");
}

int32_t comp_ywtbarray(type_info *left, type_info *right,
                       void * /*l_meta*/, void * /*r_meta*/) {
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

ssize_t get_size_ywtbarray(type_info *src, void * /*meta*/) {
    ywtBarray *val = reinterpret_cast<ywtBarray*>(src);

    if (val->len >= ywtBarray::LONG_LEN) {
        return val->len + 5;
    }

    return val->len + 1;
}

const TypeModule type_modules[] = {
    {0, 0, TYPE_FLAG_NULL, NULL, NULL, NULL, NULL, NULL, "NULL"},
    TYPE_DECL_PRIMITIVE(ywtInt,  print_ywtint),
    TYPE_DECL_PRIMITIVE(ywtLong, print_ywtlong),
    TYPE_DECL_PRIMITIVE(ywtPtr,  print_ywtptr),
    TYPE_DECL(ywtBarray, TYPE_FLAG_VAR_SIZE,
              read_ywtbarray, write_ywtbarray,
              print_ywtbarray, comp_ywtbarray,
              get_size_ywtbarray)
};

class type_test_class {
 public:
    type_test_class() {
        barray.len = 0;
    }

    template<typename T>
    void dump(T *arc) {
        arc->initialize();
        arc->dump(&val1, "val1", NULL);
        arc->dump(&val2, "val2", NULL);
        arc->dump(&val3, "val3", NULL);
        arc->dump(&ptr,  "ptr", NULL);
        arc->dump(&barray,  "barray", NULL);
        arc->finalize();
    }

    ywtInt     val1;
    ywtInt     val2;
    ywtInt     val3;
    ywtPtr     ptr;
    ywtBarray  barray;
};

void type_basic_test() {
    type_test_class  tc;
    Byte             buf[65536];

    ywt_print prt;
    ywt_read  read(sizeof(buf), buf);
    ywt_write write(sizeof(buf), buf);

    tc.val1.val = 1111;
    tc.val2.val = 2222;
    tc.val3.val = 3333;
    tc.ptr.val  = reinterpret_cast<intptr_t>(&tc);

    tc.dump(&prt);
    tc.dump(&write);

    memset(&tc, 0, sizeof(tc));

    tc.dump(&prt);
    tc.dump(&read);
    tc.dump(&prt);

    printHex(write.get_used_size(), buf, true/*info*/);
}
