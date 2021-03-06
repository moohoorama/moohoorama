/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWORDEREDNODE_H_
#define INCLUDE_YWORDEREDNODE_H_

#include <ywcommon.h>
#include <string.h>
#include <ywutil.h>
#include <ywseqlock.h>
#include <stddef.h>
#include <ywtypes.h>

typedef void    (*testFunc)(int32_t seq);

extern void null_test_func(int32_t);
extern void test_binary(int32_t seq);

extern void OrderedNode_basic_test();
extern void OrderedNode_search_test(int32_t cnt, int32_t method);
extern void OrderedNode_bsearch_insert_conc_test();
extern void OrderedNode_stress_test(int32_t thread_count);

template<typename KEY, testFunc test = null_test_func,
         typename SLOT = uint16_t, size_t PAGE_SIZE = 32*KB,
         typename HEADER = intptr_t>
class ywOrderedNode {
    typedef ywOrderedNode<KEY, test, SLOT, PAGE_SIZE, HEADER> node_type;
    static const uint32_t LOCK_MASK  = 0x1;

    static const size_t   META_SIZE = sizeof(HEADER) +
                                      sizeof(node_type*) + sizeof(uint32_t) +
                                      sizeof(SLOT)*2;
    static const uint32_t IDEAL_MAX_SLOT_COUNT =
        (PAGE_SIZE-META_SIZE)/sizeof(SLOT);

 public:
    static const SLOT     NULL_SLOT = static_cast<SLOT>(-1);

    ywOrderedNode() {
//        check_inheritance<ywTypes, KEY>();
        static_assert(META_SIZE == offsetof(ywOrderedNode, slot),
                      "invalid meta slot count");
        static_assert(sizeof(*this) == PAGE_SIZE,
                      "invalid node structure");
        static_assert(PAGE_SIZE-1 <= static_cast<SLOT>(-1),
                      "invalid PAGE_SIZE and SLOT");
        clear();
    }

    void clear() {
        new_node    = this;
        lock_seq    = 0;
        count       = 0;
        free_offset = PAGE_SIZE - META_SIZE;
    }

    volatile uint32_t *get_lock_seq_ptr() {
        return &lock_seq;
    }

    size_t  get_page_size() {
        return PAGE_SIZE;
    }
    SLOT  get_slot_size() {
        return sizeof(SLOT);
    }
    SLOT  get_meta_size() {
        return META_SIZE;
    }

    Byte *get_ptr(SLOT offset) {
        Byte * base_ptr = reinterpret_cast<Byte*>(slot);
        return base_ptr + offset;
    }

    KEY get(SLOT idx) {
        return KEY(get_ptr(slot[idx]));
    }

    SLOT    get_count() {
        return count;
    }

    SLOT get_free() {
        return free_offset - count*sizeof(SLOT);
    }

    int32_t get_size(Byte *val) {
        return KEY(val).get_size();
    }

    SLOT alloc(SLOT size) {
        assert(is_locked());

        if (get_free() >= size + sizeof(SLOT)) {
            free_offset -= size;
            return free_offset;
        }
        return NULL_SLOT;
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

#define TEST_DECLARE int32_t seq = 0
#define TEST_BLOCK   do { if (test != null_test_func) test(seq++);} while (0)
    bool insert(KEY value, bool need_locking = true) {
        return insert(&value, need_locking);
    }
    bool insert(KEY *value, bool need_locking = true) {
        ywSeqLockGuard lockGuard(&lock_seq);
        TEST_DECLARE;

        TEST_BLOCK;

        if (need_locking) {
            if (!lockGuard.lock()) {
                return false;
            }
        }
        assert(is_locked());

        TEST_BLOCK;

        SLOT ret = alloc(value->get_size());
        SLOT idx;
        SLOT i;
        if (ret == NULL_SLOT) {
            return false;
        }
        value->write(get_ptr(ret));

        TEST_BLOCK;

        idx = binary_search(value)+1;
        if (count > 0) {
            slot[count] = slot[count-1];
            i = count++;
            while ((--i) > idx) {
                slot[i] = slot[i-1];
                TEST_BLOCK;
            }
        } else {
            count++;
        }
        slot[idx] = ret;
        TEST_BLOCK;
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
        if (idx < 0) idx = 0;
        if (idx < count) {
            if (value->get_size() == get(idx).get_size()) {
                value->write(get_ptr(slot[idx]));
                return true;
            }
        }
        return false;
    }

#undef TEST_DECLARE
#undef TEST_BLOCK

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
            if ((0 <= idx) && (idx < count)) {
                if (0 == value->compare(get(idx))) {
                    return _remove(idx);
                }
            }
        }
        return false;
    }

    bool copy_node(node_type *target, SLOT begin_idx, SLOT end_idx) {
        SLOT           i;
        KEY            key;

        assert(is_locked());

        for (i = begin_idx; i < end_idx; ++i) {
            key = get(i);
            if (!target->_append(&key)) {
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
        SLOT            buf_dir[2][IDEAL_MAX_SLOT_COUNT];
        SLOT           *src = buf_dir[0];
        SLOT           *dst = buf_dir[1];
        SLOT           *ptr;
        SLOT            level;
        SLOT            left, right;
        SLOT            i;
        SLOT            j;

        if (lockGuard.lock()) {
            memcpy(src, slot, count * sizeof(SLOT));
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
                        if (0 < KEY(get_ptr(src[left])).
                                 compare(KEY(get_ptr(src[right])))) {
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
            memcpy(slot, src, count * sizeof(SLOT));
        }
    }

    bool isOrdered() {
        SLOT      i;

        for (i = 0; i < count-1; ++i) {
            KEY      left(get(i)), right(get(i+1));
            if (0 > left.compare(&right)) {
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
            KEY val = get(i);
            printf("[%5d:0x%05x] ", i, slot[i]);
            val.dump();
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
        if (ret < 0) ret = 0;
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
        if (idx < 0)  idx = 0;
        *ret = get(idx);
        if (lockGuard.read_end()) {
            return true;
        }
        return false;
    }

    int32_t binary_search(KEY key) {
        return binary_search(&key);
    }
    int32_t binary_search(KEY *key) {
        int32_t      min = 0;
        int32_t      max = count - 1;
        int32_t      mid;

        while (min <= max) {
            mid = (min + max) >> 1;
            if (0 < key->compare(KEY(get(mid)))) {
                max = mid-1;
            } else {
                min = mid+1;
            }
        }

        return (min + max) >> 1;
    }

    HEADER *get_header() {
        return &header;
    }

 private:
#define TEST_DECLARE int32_t seq = 0
#define TEST_BLOCK   do { if (test != null_test_func) test(seq++);} while (0)

    bool _remove(SLOT idx) {
        SLOT          i;
        TEST_DECLARE;

        assert(is_locked());

        TEST_BLOCK;
        if (count <= idx) {
            return false;
        }

        TEST_BLOCK;

        for (i = idx; i < count-1; ++i) {
            slot[i] = slot[i+1];
            TEST_BLOCK;
        }
        --count;

        return true;
    }

    bool _append(KEY *value) {
        assert(is_locked());

        SLOT ret = alloc(value->get_size());
        if (ret == NULL_SLOT) {
            return false;
        }
        value->write(get_ptr(ret));
        slot[count++] = ret;
        return true;
    }
#undef TEST_DECLARE
#undef TEST_BLOCK

    bool is_locked() {
        return lock_seq & LOCK_MASK;
    }

    HEADER             header;
    node_type         *new_node;
    volatile uint32_t  lock_seq;
    SLOT               count;
    SLOT               free_offset;
    SLOT               slot[IDEAL_MAX_SLOT_COUNT];

    friend void OrderedNode_bsearch_insert_conc_test();
    friend void test_binary(int32_t);
};


#endif  // INCLUDE_YWORDEREDNODE_H_
