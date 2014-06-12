#include <ywtypes.h>

#define TYPE_DECL(ID, TYPE, READ, WRITE, PRINT, GET_IMG_SIZE) \
        {ID, (sizeof(TYPE)+3)/4, READ, WRITE, PRINT, GET_IMG_SIZE, #TYPE}

template<ssize_t val>
ssize_t constant_f(type_info * /*src*/) {
    return val;
}

const TypeModule type_modules[] = {
    TYPE_DECL(0, ywtInt, NULL, NULL, NULL, constant_f<4>),
    TYPE_DECL(1, ywtFloat, NULL, NULL, NULL, constant_f<8>)
}

typedef struct {
    int32_t val;
} ywtInt;

typedef struct {
    float val;
} ywtFloat;


