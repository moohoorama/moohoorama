/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWFSNODE_H_
#define INCLUDE_YWFSNODE_H_

#include <ywcommon.h>
#include <string.h>
#include <ywutil.h>
#include <ywseqlock.h>
#include <stddef.h>
#include <ywtypes.h>

extern void FSNode_basic_test();
extern void FSNode_search_test(int32_t cnt, int32_t method);
extern void FSNode_bsearch_insert_conc_test();
extern void FSNode_stress_test(int32_t thread_count);

template<typename KEY, size_t PAGE_SIZE = 32*KB, typename HEADER = intptr_t,
    bool bin_search = true>
class ywFSNode {
    typedef ywFSNode<KEY, PAGE_SIZE, HEADER> node_type;
    typedef uint32_t                         SLOT;
    static const uint32_t LOCK_MASK  = 0x1;

    static const size_t   CORE_SIZE = sizeof(HEADER) +
                                      sizeof(node_type*) + sizeof(uint32_t) +
                                      sizeof(SLOT)*3;
    static const size_t   META_SIZE = 64;
    static const size_t   PAD_SIZE  = 64 - CORE_SIZE;
    static const size_t   KEY_SIZE = sizeof(KEY);
    static const uint32_t IDEAL_MAX_SLOT_COUNT = (PAGE_SIZE-META_SIZE)/KEY_SIZE;

 public:
    static const SLOT     NULL_SLOT = static_cast<SLOT>(-1);

    ywFSNode() {
        static_assert(META_SIZE == offsetof(ywFSNode, slot),
                      "invalid meta slot count");
        static_assert(sizeof(*this) == PAGE_SIZE, "invalid node structure");
        static_assert(PAGE_SIZE-1 <= static_cast<SLOT>(-1),
                      "invalid PAGE_SIZE and SLOT");
        clear();
    }

    void clear() {
        new_node    = this;
        lock_seq    = 0;
        count       = 0;
        free_offset = PAGE_SIZE - META_SIZE;
        free_size   = 0;
    }

    volatile uint32_t *get_lock_seq_ptr() {
        return &lock_seq;
    }

    size_t  get_page_size() {
        return PAGE_SIZE;
    }
    size_t  get_slot_size() {
        return sizeof(KEY);
    }
    size_t  get_meta_size() {
        return META_SIZE;
    }

    SLOT get_count() {
        return count;
    }

    SLOT get_free() {
        return free_offset - count*KEY_SIZE;
    }

    KEY get(SLOT idx) {
        return slot[idx];
    }

    int32_t get_size(Byte *val) {
        return KEY(val).get_size();
    }

    bool append(KEY *value) {
        ywSeqLockGuard lockGuard(&lock_seq);
        if (!lockGuard.lock()) {
            return false;
        }

        return _append(value);
    }
    bool append(KEY value) {
        return append(&value);
    }

    bool insert(KEY value, bool need_locking = true) {
        return insert(&value, need_locking);
    }
    bool insert(KEY *value, bool need_locking = true) {
        ywSeqLockGuard lockGuard(&lock_seq);
        int32_t        idx;

        if (need_locking) {
            if (!lockGuard.lock()) {
                return false;
            }
        }
        assert(is_locked());

        if (count >= IDEAL_MAX_SLOT_COUNT) return false;

        if (count > 0) {
            if (bin_search) {
                idx = binary_search(value)+1;
            } else {
                idx = binary_search(value);
                if (0 >= value->compare(&slot[idx])) {
                    idx++;
                }
            }

            slot[count] = slot[count-1];
            int32_t i = count++;
            while ((--i) > idx) {
                slot[i] = slot[i-1];
            }
        } else {
            idx = 0;
            count++;
        }
        slot[idx] = *value;

        return true;
    }

    bool inplace_update(KEY value) {
        return inplace_update(&value);
    }
    bool inplace_update(KEY *value) {
        ywSeqLockGuard lockGuard(&lock_seq);
        int32_t        idx;

        if (!lockGuard.lock()) {
            return false;
        }
        idx = binary_search(value);
        if (idx < 0)      idx = 0;
        if (idx >= count) idx = count - 1;
        if (static_cast<SLOT>(idx) < count) {
            slot[idx] = *value;
            return true;
        }
        return false;
    }

    bool remove(SLOT idx) {
        ywSeqLockGuard lockGuard(&lock_seq);

        if (lockGuard.lock()) {
            return _remove(idx);
        }

        return false;
    }

    bool remove(KEY value) {
        return remove(&value);
    }

    bool remove(KEY *value) {
         ywSeqLockGuard lockGuard(&lock_seq);
         SLOT           idx;

        if (lockGuard.lock()) {
            idx = binary_search(value);
            if (idx < 0)      idx = 0;
            if (idx >= count) idx = count - 1;
            if (0 == value->compare(slot[idx])) {
                return _remove(idx);
            }
        }
        return false;
    }

    bool copy_node(node_type *target, SLOT begin_idx, SLOT end_idx) {
        SLOT           i;

        assert(is_locked());

        for (i = begin_idx; i < end_idx; ++i) {
            if (!target->_append(&slot[i])) {
                return false;
            }
        }

        return true;
    }

    bool compact(node_type *target) {
        ywSeqLockGuard  lockGuard(&lock_seq);
        if (lockGuard.lock()) {
            target->clear();
            ywSeqLockGuard  lockGuard2(&target->lock_seq);
            if (lockGuard2.lock()) {
                if (copy_node(target, 0, count)) {
                    new_node = target;
                    return true;
                }
            }
        }
        return false;
    }

    void sort() {
        ywSeqLockGuard  lockGuard(&lock_seq);
        KEY             buf_dir[2][IDEAL_MAX_SLOT_COUNT];
        KEY            *src = buf_dir[0];
        KEY            *dst = buf_dir[1];
        KEY            *ptr;
        SLOT            level;
        SLOT            left, right;
        SLOT            i;
        SLOT            j;

        if (lockGuard.lock()) {
            memcpy(src, slot, count * KEY_SIZE);
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
                        if (0 < src[left].compare(src[right])) {
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
            memcpy(slot, src, count * KEY_SIZE);
        }
    }

    bool isOrdered() {
        SLOT      i;

        for (i = 0; i < count-1; ++i) {
            if (0 > slot[i].compare(slot[i+1])) {
                return false;
            }
        }

        return true;
    }

    /*
       SLOT interpolation_search(Byte *key) {
       int32_t val = *reinterpret_cast<int*>(key);
       int32_t left, right;
       int32_t pos, idx, lidx = 0, ridx = count-1;;
       int32_t ret;

       while (lidx < ridx) {
       left  = *reinterpret_cast<int*>(get_slot(lidx));
       right = *reinterpret_cast<int*>(get_slot(ridx));
       pos = (val-left) * (ridx-lidx) / (right - left);
       idx = lidx + pos;
       ret = comp(key, get_slot(idx));
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

       ret = comp(key, get_slot(lidx));
       if (0 < ret) {
       return lidx -1;
       }
       return lidx;
       }
     */

    void dump() {
        SLOT i;

        printf("count:%-5d free_offset:%-5d free:%-5d\n",
               count,
               free_offset,
               get_free());
        for (i = 0; i < count; ++i) {
            printf("[%5] ", i);
            slot[i].dump();
            printf("\n");
        }
    }
    int32_t search(KEY key) {
        return search(&key);
    }
    int32_t search(KEY *key) {
        ywSeqLockGuard  lockGuard(&lock_seq);
        int32_t         ret;
        lockGuard.read_begin();
        ret = binary_search(key);
        if (ret < 0)      ret = 0;
        if (ret >= count) ret = count - 1;
        if (lockGuard.read_end()) {
            return ret;
        }
        return NULL_SLOT;
    }

    bool search_body(KEY key, KEY *ret) {
        return search_body(&key, ret);
    }
    bool search_body(KEY *key, KEY *ret) {
        ywSeqLockGuard  lockGuard(&lock_seq);
        int32_t         idx;

        lockGuard.read_begin();
        idx = binary_search(key);
        if (idx < 0)      idx = 0;
        if (idx >= count) idx = count - 1;
        *ret = slot[idx];
        if (lockGuard.read_end()) {
            return true;
        }
        return false;
    }

    int32_t binary_search(KEY key) {
        return binary_search(&key);
    }
    int32_t binary_search(KEY *key) {
        if (bin_search) {
            int32_t      min = 0;
            int32_t      max = count - 1;
            int32_t      mid;

            while (min <= max) {
                mid = (min + max) >> 1;
                if (0 < key->compare(slot[mid])) {
                    max = mid-1;
                } else {
                    min = mid+1;
                }
            }

            return (min + max) >> 1;
        } else {
            int32_t      idx;
            int32_t      begin = 0;
            int32_t      size  = count;
            int32_t      half  = size / 2;

            do {
                idx = begin + half;
                if (0 < key->compare(&slot[idx])) {
                    size = half;
                } else {
                    begin+= half;
                    size -= half;
                }
                half = size/2;
            } while (half > 0);

            return begin;
        }
    }

    HEADER *get_header() {
        return &header;
    }

 private:
    bool _remove(SLOT idx) {
        SLOT          i;

        assert(is_locked());

        if (count <= idx) {
            return false;
        }


        for (i = idx; i < count-1; ++i) {
            slot[i] = slot[i+1];
        }
        --count;

        return true;
    }

    bool _append(KEY *value) {
        assert(is_locked());

        if (count >= IDEAL_MAX_SLOT_COUNT) return false;
        slot[count++] = *value;
        return true;
    }

    bool is_locked() {
        return lock_seq & LOCK_MASK;
    }

    HEADER             header;
    node_type         *new_node;
    volatile uint32_t  lock_seq;
    SLOT               count;
    SLOT               free_offset;
    SLOT               free_size;
    char               pad[PAD_SIZE];
    KEY                slot[IDEAL_MAX_SLOT_COUNT];

    friend void FSNode_bsearch_insert_conc_test();
    friend void test_binary(int32_t);
};


#endif  // INCLUDE_YWFSNODE_H_
