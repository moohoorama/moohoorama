/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWORDEREDNODE_H_
#define INCLUDE_YWORDEREDNODE_H_

#include <ywcommon.h>
#include <string.h>
#include <ywutil.h>
#include <stddef.h>

typedef int32_t (*compareFunc)(char *left, char *right);

template<typename SLOT = uint16_t, SLOT PAGE_SIZE = 32*KB>
class ywOrderedNode {
 public:
    static const SLOT NULL_SLOT = static_cast<SLOT>(-1);
    static const SLOT META_SLOT_COUNT = 2;
    static const SLOT IDEAL_MAX_SLOT_COUNT =
                           PAGE_SIZE/sizeof(SLOT) - META_SLOT_COUNT;

    ywOrderedNode() {
        clear();
        static_assert(META_SLOT_COUNT ==
                      offsetof(ywOrderedNode, slot)/sizeof(SLOT),
                      "invalid meta slot count");
        static_assert(sizeof(*this) == PAGE_SIZE,
                      "invalid node structure");
        static_assert(PAGE_SIZE <= static_cast<SLOT>(-1),
                      "invalid PAGE_SIZE and SLOT");
    }

    void clear() {
        count       = 0;
        free_offset = PAGE_SIZE - sizeof(SLOT);
    }

    SLOT get_free() {
        return free_offset - (count + META_SLOT_COUNT) * sizeof(SLOT);
    }

    SLOT alloc(SLOT size) {
        SLOT ret = NULL_SLOT;
        if (get_free() >= size + sizeof(SLOT)) {
            free_offset -= size;
            slot[count] = free_offset;
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
        return get_slot_by_offset(slot[idx]);
    }
    char *get_slot_by_offset(SLOT offset) {
        char * base_ptr = reinterpret_cast<char*>(&count);
        return base_ptr + offset;
    }

    template<compareFunc func>
    void sort() {
        SLOT buf_dir[2][IDEAL_MAX_SLOT_COUNT];
        SLOT *src = buf_dir[0];
        SLOT *dst = buf_dir[1];
        SLOT *ptr;
        SLOT level;
        SLOT left, right;
        SLOT i;
        SLOT j;

        memcpy(src, slot, sizeof(SLOT)*count);
        for (level = 2; level/2 < count; level*=2) {
            j = 0;
            for (i = 0; i < count; i += level) {
                left = i;
                right = i + level/2;
                if (right >= count) {
                    while ((j < i+level) && (j < count)) {
                        dst[j++] = src[left++];
                    }
                    break;
                }

                do {
                    if (0 < func(get_slot_by_offset(src[left]),
                                 get_slot_by_offset(src[right]))) {
                        dst[j++] = src[left++];
                        if (left == i + level/2) {
                            while ((j < i+level) && (j < count)) {
                                dst[j++] = src[right++];
                            }
                            break;
                        }
                    } else {
                        dst[j++] = src[right++];
                        if ((right == i + level) || (right == count)) {
                            while ((j < i+level) && (j < count)) {
                                dst[j++] = src[left++];
                            }
                            break;
                        }
                    }
                } while ((j < i+level) && (j < count));
            }
            /* Flipping*/
            ptr = dst; dst = src; src = ptr;
        }
        memcpy(slot, src, sizeof(SLOT)*count);
    }

    template<compareFunc func>
    bool isOrdered() {
        SLOT i;
        for (i = 0; i < count-1; ++i) {
            if (0 > func(get_slot(i), get_slot(i+1))) {
                return false;
            }
        }

        return true;
    }

    template<compareFunc func>
    SLOT binary_search(char *key) {
        SLOT base = 0, size = count, half, idx;

        do {
            half = size/2;
            idx = base + half;
            if (0 < func(key, get_slot(idx))) {
                size = half;
            } else {
                base = idx;
                size -= half;
            }
        } while (half > 0);

        return base;
    }

    template<compareFunc func>
    SLOT interpolation_search(char *key) {
        int32_t val = *reinterpret_cast<int*>(key);
        int32_t left, right;
        int32_t pos, idx, lidx = 0, ridx = count-1;;
        int32_t ret;

        while (lidx < ridx) {
            left  = *reinterpret_cast<int*>(get_slot(lidx));
            right = *reinterpret_cast<int*>(get_slot(ridx));
            pos = (val-left) * (ridx-lidx) / (right - left);
            idx = lidx + pos;
            ret = func(key, get_slot(idx));
            if (ret == 0) {
                return idx;
            }
            if (0 < ret) {
                ridx = pos-1;
            } else {
                lidx = pos+1;
            }
        }

        if (lidx != ridx) {
            return (lidx + ridx)>>1;
        }

        ret = func(key, get_slot(lidx));
        if (0 < ret) {
            return lidx -1;
        }
        return lidx;
    }

    void dump() {
        SLOT i;

        printf("count:%-5d free_offset:%-5d\n",
               count,
               free_offset);
        for (i = 0; i < count; ++i) {
            printf("[%5d:0x%05x] ", i, slot[i]);
            printHex(4, get_slot(i), false/*info*/);
            printf("\n");
        }
    }

 private:
    SLOT count;
    SLOT free_offset;
    SLOT slot[IDEAL_MAX_SLOT_COUNT];
};

extern void OrderedNode_basic_test(int32_t cnt, int32_t method);

#endif  // INCLUDE_YWORDEREDNODE_H_
