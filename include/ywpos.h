/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWPOS_H_
#define INCLUDE_YWPOS_H_

#include <ywcommon.h>

class ywPos {
 public:
    explicit ywPos():_val(0) { }
    explicit ywPos(uint64_t val):_val(val) { }
    bool  is_null() { return _val == 0;}
    uint64_t get()  { return _val;}
    void set_null() { set(0); }
    void set(uint64_t val) { _val = val; }

    template<typename T>
        bool cas(T *ptr, T prev, T next) {
            return __sync_bool_compare_and_swap(ptr, prev, next);
        }

    template<size_t ALIGN>
    ywPos go_forward(size_t size) {
        uint64_t prev;
        uint32_t off;

        do {
            prev = _val;

            if (ALIGN) {
                off = prev & (ALIGN-1);

                while (off + size > ALIGN) {
                    if (_cas_next_align<ALIGN>(prev)) {
                        prev = _val;
                        break;
                    }
                    prev = _val;
                    off = prev & (ALIGN-1);
                }
            }
        } while (!cas(&_val, prev, prev + size));

        return ywPos(prev);
    }

 private:
    template<size_t ALIGN>
    bool _cas_next_align(uint64_t prev) {
        static_assert((ALIGN & (~ALIGN)) == 0, "in_valid align size");
        uint64_t next = ((prev + (ALIGN-1)) & ~(ALIGN-1));
        return cas(&_val, prev, next);
    }

    uint64_t _val;
};

#endif  // INCLUDE_YWPOS_H_
