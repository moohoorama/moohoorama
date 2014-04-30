/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWPOS_H_
#define INCLUDE_YWPOS_H_

#include <ywcommon.h>

class ywPos {

 public:
    explicit ywPos():val(0) { }
    explicit ywPos(uint64_t _val):val(_val) { }
    bool  is_null() { return val == 0;}
    uint64_t get()  { return val;}
    void set_null() { set(0); }
    void set(uint64_t _val) { val = _val; }

    template<typename T>
        bool cas(T *ptr, T prev, T next) {
            return __sync_bool_compare_and_swap(ptr, prev, next);
        }

    template<size_t ALIGN>
    ywPos alloc(size_t size) {
        uint64_t prev;
        uint32_t off;

        do {
            prev = val;

            if (ALIGN) {
                off = prev & (ALIGN-1);

                while (off + size > ALIGN) {
                    if (cas_next_align<ALIGN>(prev)) {
                        prev = val;
                        break;
                    }
                    prev = val;
                    off = prev & (ALIGN-1);
                }
            }
        } while (!cas(&val, prev, prev + size));

        return ywPos(prev);
    }

    template<size_t ALIGN>
    bool cas_next_align(uint64_t prev) {
        static_assert((ALIGN & (~ALIGN)) == 0, "invalid align size");
        uint64_t next = ((prev + (ALIGN-1)) & ~(ALIGN-1)) + ALIGN; 
        return cas(&val, prev, next);
    }

 private:
    uint64_t val;
};



#endif  // INCLUDE_YWPOS_H_

