/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWORDEREDNODE_H_
#define INCLUDE_YWORDEREDNODE_H_

#include <ywcommon.h>
#include <string.h>
#include <ywutil.h>

template<typename SLOT = uint16_t, SLOT PAGE_SIZE = 32*KB>
class ywOrderedNode {
 public:
    static const SLOT NULL_SLOT = static_cast<SLOT>(-1);
    static const SLOT IDEAL_MAX_SLOT_COUNT = PAGE_SIZE/sizeof(SLOT) - 2;

    ywOrderedNode() {
        count       = 0;
        last_offset = PAGE_SIZE - sizeof(SLOT);
        static_assert(sizeof(*this) == PAGE_SIZE,
                      "invalid node structure");
        static_assert(PAGE_SIZE <= static_cast<SLOT>(-1),
                      "invalid PAGE_SIZE and SLOT");
    }

    SLOT get_right_free() {
        return slot[count-1];
    }

    SLOT get_left_free() {
        return (count+2)*sizeof(SLOT);
    }

    SLOT get_free() {
        return get_right_free() - get_left_free();
    }

    SLOT alloc(SLOT size) {
        SLOT ret = NULL_SLOT;
        if (get_free() >= size + sizeof(SLOT)) {
            slot[count] = get_right_free() - size;
            ret = count;
            count++;
        }
        return ret;
    }

    template<typename T>
    bool insert(T *val) {
        SLOT ret = alloc(sizeof(*val));
        if (ret != NULL_SLOT) {
            memcpy(get_slot(ret), val, sizeof(*val));
            return true;
        }
        return false;
    }

    char *get_slot(SLOT idx) {
        char * base_ptr = reinterpret_cast<char*>(&count);
        return base_ptr + slot[idx];
    }

    void dump() {
        SLOT i;

        printf("count:%-5d last_offset:%-5d\n",
               count,
               last_offset);
        for (i = 0; i < count; ++i) {
            printf("[%3d] 0x%05x ", i, slot[i]);
            printVar(" ", slot[i-1] - slot[i], get_slot(i));
        }
    }

 private:
    SLOT count;
    SLOT last_offset;
    SLOT slot[IDEAL_MAX_SLOT_COUNT];
};

extern void OrderedNode_basic_test();

#endif  // INCLUDE_YWORDEREDNODE_H_
