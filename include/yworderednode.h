/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWORDEREDNODE_H_
#define INCLUDE_YWORDEREDNODE_H_

#include <ywcommon.h>
#include <string.h>
#include <ywutil.h>
#include <ywseqlock.h>
#include <stddef.h>

typedef int32_t (*compFunc)(char *left, char *right);
typedef int32_t (*sizeFunc)(char *ptr);
typedef void    (*testFunc)(int32_t seq);

extern void null_test_func(int32_t);
extern void test_binary(int32_t seq);

extern void OrderedNode_basic_test();
extern void OrderedNode_search_test(int32_t cnt, int32_t method);
extern void OrderedNode_bsearch_insert_conc_test();
extern void OrderedNode_stress_test(int32_t thread_count);

template<compFunc comp, sizeFunc get_size, testFunc test = null_test_func,
         typename SLOT = uint16_t, size_t PAGE_SIZE = 32*KB,
         typename APPENDIX_HEADER = intptr_t>
class ywOrderedNode {
    typedef ywOrderedNode<comp, get_size, test,
            SLOT, PAGE_SIZE, APPENDIX_HEADER> node_type;
    static const uint32_t LOCK_MASK  = 0x1;

    static const SLOT     NULL_SLOT = static_cast<SLOT>(-1);
    static const size_t   META_SIZE = sizeof(APPENDIX_HEADER) +
                                      sizeof(node_type*) + sizeof(uint32_t) +
                                      sizeof(SLOT)*2;
    static const uint32_t IDEAL_MAX_SLOT_COUNT =
        (PAGE_SIZE-META_SIZE)/sizeof(SLOT);

 public:
    ywOrderedNode() {
        clear();
        static_assert(META_SIZE == offsetof(ywOrderedNode, slot),
                      "invalid meta slot count");
        static_assert(sizeof(*this) == PAGE_SIZE,
                      "invalid node structure");
        static_assert(PAGE_SIZE-1 <= static_cast<SLOT>(-1),
                      "invalid PAGE_SIZE and SLOT");
    }

    void clear() {
        new_node    = this;
        lock_seq    = 0;
        count       = 0;
        free_offset = PAGE_SIZE - META_SIZE;
    }

    volatile uint32_t *get_lock_seq_ptr() {
        return lock_seq;
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

    char *get_ptr(SLOT offset) {
        char * base_ptr = reinterpret_cast<char*>(slot);
        return base_ptr + offset;
    }

    char *get_slot(SLOT idx) {
        return get_ptr(slot[idx]);
    }

    SLOT get_count() {
        return count;
    }

    SLOT get_free() {
        return free_offset - count*sizeof(SLOT);
    }

    SLOT alloc(SLOT size) {
        assert(is_locked());

        if (get_free() >= size + sizeof(SLOT)) {
            free_offset -= size;
            return free_offset;
        }
        return NULL_SLOT;
    }

    bool append(size_t size, char * value) {
        ywSeqLockGuard lockGuard(&lock_seq);
        if (!lockGuard.lock()) {
            return false;
        }

        SLOT ret = alloc(size);
        if (ret == NULL_SLOT) {
            return false;
        }
        memcpy(get_ptr(ret), value, size);
        slot[count++] = ret;
        return true;
    }

    template<typename T> bool append(T *val) {
        return append(sizeof(*val), reinterpret_cast<char*>(val));
    }

#define TEST_DECLARE int32_t seq = 0
#define TEST_BLOCK   do { if (test != null_test_func) test(seq++);} while (0)
    bool insert(size_t size, char * value) {
        ywSeqLockGuard lockGuard(&lock_seq);
        TEST_DECLARE;

        TEST_BLOCK;

        if (!lockGuard.lock()) {
            return false;
        }
        assert(is_locked());

        TEST_BLOCK;

        SLOT ret = alloc(size);
        SLOT idx;
        SLOT i;
        if (ret == NULL_SLOT) {
            return false;
        }
        memcpy(get_ptr(ret), value, size);

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
#undef TEST_DECLARE
#undef TEST_BLOCK

    template<typename T> bool insert(T *val) {
        return insert(sizeof(*val), reinterpret_cast<char*>(val));
    }

    bool remove(SLOT idx) {
        ywSeqLockGuard lockGuard(&lock_seq);

        if (lockGuard.lock()) {
            return _remove(idx);
        }

        return false;
    }

    bool remove(size_t size, char * value) {
        ywSeqLockGuard lockGuard(&lock_seq);
        SLOT           idx;

        if (lockGuard.lock()) {
            idx = binary_search(value);
            if ((0 <= idx) && (idx < count)) {
                if (0 == comp(get_slot(idx), value)) {
                    return _remove(idx);
                }
            }
        }
        return false;
    }

    template<typename T> bool remove(T *val) {
        return remove(sizeof(*val), reinterpret_cast<char*>(val));
    }

    bool copy_node(node_type *target, int begin_idx) {
        char          *slot;
        SLOT           i;

        assert(is_locked());

        target->clear();

        for (i = begin_idx; i < count; ++i) {
            slot = get_slot(i);
            if (!target->append(get_size(slot), slot)) {
                return false;
            }
        }

        return true;
    }

    bool compact(node_type *target) {
        ywSeqLockGuard  lockGuard(&lock_seq);
        if (lockGuard.lock()) {
            if (copy_node(target, 0)) {
                new_node = target;
                return true;
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
                        if (0 < comp(get_ptr(src[left]),
                                     get_ptr(src[right]))) {
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
            if (0 > comp(get_slot(i), get_slot(i+1))) {
                return false;
            }
        }

        return true;
    }

    /*
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
            printf("[%5d:0x%05x] ", i, slot[i]);
            printHex(4, get_slot(i), false/*info*/);
            printf("\n");
        }
    }
    int32_t search(char *key) {
        ywSeqLockGuard  lockGuard(&lock_seq);
        int32_t         ret;
        lockGuard.read_begin();
        ret = binary_search(key);
        if (lockGuard.read_end()) {
            return ret;
        }
        return NULL_SLOT;
    }

    char *search_body(char *key) {
        ywSeqLockGuard  lockGuard(&lock_seq);
        int32_t         idx;
        char           *ret;

        lockGuard.read_begin();
        idx = binary_search(key);
        ret = get_slot(idx);
        if (lockGuard.read_end()) {
            return ret;
        }
        return NULL;
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
#undef TEST_DECLARE
#undef TEST_BLOCK

    int32_t binary_search(char *key) {
        int32_t      min = 0;
        int32_t      max = count - 1;
        int32_t      mid;

        while (min <= max) {
            mid = (min + max) >> 1;
            if (0 < comp(key, get_slot(mid))) {
                max = mid-1;
            } else {
                min = mid+1;
            }
        }

        return (min + max) >> 1;
    }

    bool is_locked() {
        return lock_seq & LOCK_MASK;
    }

    APPENDIX_HEADER    appendix_header;
    node_type         *new_node;
    volatile uint32_t  lock_seq;
    SLOT               count;
    SLOT               free_offset;
    SLOT               slot[IDEAL_MAX_SLOT_COUNT];

    friend void OrderedNode_bsearch_insert_conc_test();
    friend void test_binary(int32_t);
};


#endif  // INCLUDE_YWORDEREDNODE_H_
