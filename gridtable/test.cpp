#include <stdio.h>
#include "gridtable.h"

__attribute__((noinline)) void *get_pc() {
    return __builtin_return_address(0);
}

typedef uint8_t Byte;

typedef int32_t ywInt;
typedef float   ywFloat;

const int32_t try_count = 1000*1000*100;

template<typename func_ptr>
void call(func_ptr func) {
    int i =0;
    int val =1;

    for (i = 0 ; i < try_count; ++i) {
        func(&val);
    }
    printf("%d\n", val);
}

__attribute__((always_inline))
inline void increase(int *val) {
    ++(*val);
    *val = *val * 3421 + *val;
}

void inline_call() {
    int i =0;
    int val =1;

    for (i = 0 ; i < try_count; ++i) {
        increase(&val);
    }
    printf("%d\n", val);
}

void test_(int a) {
    printf("%d\n", a);
}

void test__(int a, int b) {
    printf("%d %d\n", a, b);
}

/*
template<typename func>
bool call_type(int32_t id, Byte *buf) {
    switch(id) {
    case 0:
        {
            ywInt val = *reinterpret_cast<ywInt*>(buf);
            func(val);
        }
        break;
    case 1:
        {
            ywFloat val = *reinterpret_cast<ywFloat*>(buf);
            func(val);
        }
        break;
    }

    return true;
}*/

typedef void (*test_func_1)(int var);
typedef void (*test_func_2)(int var, int var2);



int main(int argc, char **argv) {
    GridTable<HtmlRenderer> gt;
    test_func_2 func_ptr = reinterpret_cast<test_func_2>(test_);
    
    func_ptr(4,5);
    call(increase);
    
//    inline_call();

    return 0;

    gt.print();
    gt.set(0,0,"abc");
    gt.print();

    gt.set(0,0,"name");
    gt.set(1,0,"tel");
    gt.set(2,0,"addr");
    gt.print();

    gt.set(0,0,"no");
    gt.set(1,0,"name");
    gt.set(2,0,"tel");
    gt.set(3,0,"addr");
    gt.print();

    gt.set(0, 1, 1);
    gt.set(1, 1, "Younwoo");
    gt.set(2, 1, "010-2565-3903");
    gt.set(3, 1, "Ahnyang");
    gt.print();

    gt.set(0, 2, 1);
    gt.set(1, 2, "Younwoo");
    gt.set(2, 2, "010-2565-3903");
    gt.set(3, 2, "Ahnyang");
    gt.set_bar(1);
    gt.set(1, 3, "[               ]");
    gt.print();

    return 0;
}
