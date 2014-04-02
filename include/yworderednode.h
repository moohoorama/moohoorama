/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWORDEREDNODE_H_
#define INCLUDE_YWORDEREDNODE_H_

#include <ywcommon.h>
#include <string.h>
#include <ywutil.h>
#include <stddef.h>

typedef int32_t (*compareFunc)(char *left, char *right);


class ywONLockGuard;

template<compareFunc func,typename SLOT = uint16_t, SLOT PAGE_SIZE = 32*KB>
class ywOrderedNode {
 public:
    static const SLOT   NULL_SLOT = static_cast<SLOT>(-1);
    static const size_t META_SIZE = sizeof(SLOT)*2;
    static const SLOT   IDEAL_MAX_SLOT_COUNT =
                           (PAGE_SIZE-META_SIZE)/sizeof(SLOT);
    static const SLOT   LOCK_MASK  = (NULL_SLOT/2) + 1;
    static const SLOT   COUNT_MASK = ~LOCK_MASK;

    ywOrderedNode() {
        clear();
        static_assert(META_SIZE == offsetof(ywOrderedNode, slot),
                      "invalid meta slot count");
        static_assert(sizeof(*this) == PAGE_SIZE,
                      "invalid node structure");
        static_assert(PAGE_SIZE <= static_cast<SLOT>(-1),
                      "invalid PAGE_SIZE and SLOT");
        static_assert(COUNT_MASK+1 == LOCK_MASK, "invalid lock/count mask");
    }

    void clear() {
        count       = 0;
        free_offset = PAGE_SIZE;
    }

    char *get_ptr(SLOT offset) {
        char * base_ptr = reinterpret_cast<char*>(&this);
        return base_ptr + offset;
    }

    SLOT get_count() {
        return count & COUNT_MASK;
    }

    SLOT get_free() {
        return free_offset - count*sizeof(SLOT) - META_SIZE;
    }

    SLOT alloc(SLOT size) {
        SLOT ret = NULL_SLOT;

        assert(count & LOCK_MASK);

        if (get_free() >= size + sizeof(SLOT)) {
            free_offset -= size;
            return free_offset;
        }
        return NULL_SLOT;
    }

    bool append(size_t size, char * value) {
        ywONLockGuard lockGuard(this);
        if (lockGuard.lock()) {
            assert(count & LOCK_MASK);

            SLOT ret = alloc(size);
            if (ret != NULL_SLOT) {
                memcpy(get_ptr(ret), value, size);
                slot[count++] = ret;
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool append(T *val) {
        return append(sizeof(*val), val);
    }

    /*
    bool ordered_insert(size_t size, char * value) {
        ywONLockGuard lockGuard(this);
        if (this->lock()) {
            uint32_t side     = get_side();
            assert(side & LOCK_MASK);

            SLOT ret = alloc(size);
            SLOT idx;
            if (ret != NULL_SLOT) {
                memcpy(ret, value, size);
//                binary_search
                idx = meta.count[side];
                *get_slot(side, idx) = ret;
                meta.count[side]++;
                return true;
            }
        }
        return false;
    }
    */

    void sort() {
        ywONLockGuard lockGuard(this);
        SLOT      buf_dir[2][IDEAL_MAX_SLOT_COUNT];
        SLOT     *src = buf_dir[0];
        SLOT     *dst = buf_dir[1];
        SLOT     *ptr;
        SLOT      level;
        SLOT      left, right;
        SLOT      i;
        SLOT      j;
        uint32_t  side     = get_side();
        uint32_t  opposite = 1 - side;

        if (lockGuard.lock()) {
            for (i = 0; i < meta.count[side]; ++i) {
                src[i] = *get_slot(side, i);
            }
            /* merge sort*/
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
            for (i = 0; i < meta.count[side]; ++i) {
                get_slot(opposide, i) = src[i];
            }
            lockGuard.flip_and_release();
        }
    }

    bool isOrdered() {
        uint32_t  side     = get_side();
        SLOT      i;

        for (i = 0; i < count-1; ++i) {
            if (0 > func(get_slot(side, i), get_slot(side, i+1))) {
                return false;
            }
        }

        return true;
    }

    SLOT binary_search(char *key) {
        uint32_t  side = get_side();
        SLOT      base = 0, size = count, half, idx;

        do {
            half = size/2;
            idx = base + half;
            if (0 < func(key, get_slot(side, idx))) {
                size = half;
            } else {
                base = idx;
                size -= half;
            }
        } while (half > 0);

        return base;
    }

    /*
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
    */

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
    bool tryLock() {
        int32_t prev = count;
        if (prev & LOCK_MASK) return false;
        return __sync_bool_compare_and_swap(&meta.side, prev, prev | LOCK_MASK);
    }
    void release() {
        int32_t prev = count;
        assert(__sync_bool_compare_and_swap(&meta.side, prev, prev & ~LOCK_MASK);
    }

    SLOT      count;
    SLOT      free_offset;
    SLOT      slot[IDEAL_MAX_SLOT_COUNT];

    friend class ywONLockGuard;
};

class ywONLockGuard {
 public:
    explicit ywONLockGuard(ywOrderedNode *_node):node(_node), locked(false) {
    }
    ~ywONLockGuard() {
        if (locked) {
            node->release();
            locked = false;
        }
    }
    bool lock() {
        assert(locked == false);
        if (node->tryLock()) {
            locked = true;
        }
        return locked;
    }
    void flip_and_release() {
        assert(locked == true);
        node->flip_and_release();
        locked = false;
    }

 private:
    bool           locked;
    ywOrderedNode *node;
};

extern void OrderedNode_basic_test(int32_t cnt, int32_t method);

#endif  // INCLUDE_YWORDEREDNODE_H_
