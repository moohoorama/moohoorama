/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWBTREE_H_
#define INCLUDE_YWBTREE_H_

#include <ywcommon.h>
#include <yworderednode.h>
#include <ywsl.h>
#include <ywmempool.h>
#include <ywrcupool.h>
#include <ywkey.h>
#include <ywbarray.h>

class ywBTreeHeader {
 public:
    ywDList   list;
    bool      is_leaf;
    uint64_t  smo_seq;
};

template<typename KEY, typename VAL = ywPtr, size_t PAGE_SIZE = 32*KB>
class ywBTree {
 public:
    static const MAX_DEPTH = 16;

    typedef ywKey<KEY, VAL> key_type;
    typedef ywOrderedNode<key_type, null_test_func,
            uint16_t, PAGE_SIZE, ywBTreeHeader> node_type;
    class ywBTreeCursor {
        ywBTreeCursor(node_type *_node, int32_t _idx):
            node(_node), idx(_idx) {
            }
        node_type *node;
        int32_t    idx;
    };
    typedef ywStack<ywBTreeCursor, MAX_DEPTH> stack_type;

    ywBTree() {
        check_inheritance<ywTypes, KEY>();
        check_inheritance<ywTypes, VAL>();
        clear();
    }

    bool insert(KEY key, void *value) {
        return insert(key, ywPtr(value));
    }

    bool insert(KEY key, VAL value) {
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
        key_type                   keyValue(key, value);
        node_type                 *cursor = find_leaf(keyValue);

        while (!cursor->insert(keyValue)) {
            ywSeqLockGuard  guard(&smo_seq);
            ywSeqLockGuard  leafGuard(cursor->get_lock_seq_ptr());
            node_type      *new_left = rpGuard.alloc();
            node_type      *right    = rpGuard.alloc();
            int32_t         cur_count = cursor->get_count();

            if (!guard.lock()) {
                return false;
            }
            if (!cursor->copy_node(new_left, 0, cur_count/2)) return false;
            if (!cursor->copy_node(right, cur_count/2, cur_count)) return false;

            if
            cursor->split();

        }

        return true;
    }
    bool remove(Byte *key) {
        return true;
    }
    bool  find(KEY key, void *_value) {
        VAL value;
        if (find(key, &value)) {
            _value = value.get();
            return true;
        }

        return false;
    }
    void  *find(KEY key) {
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
        key_type                   keyValue(key);
        key_type                   ret;
        stack_type                 stack;
        node_type                 *cursor;
        if (!find_leaf(keyValue, &stack)) {
            return NULL;
        }

        cursor = stack.get_last().;

    bool find_leaf(key_type key, stack_type *stack) {

        while (!cursor->search_body(keyValue, &ret)) {
        }

        return ret.val.get();
    }
    void dump() {
        root->dump();
    }

 private:
    bool find_leaf(key_type key, stack_type *stack) {
        key_type   ret;
        node_type *cursor = root;

        if (!stack.push(cursor)) return false;
        while (!cursor->get_header()->is_leaf) {
            while (!cursor->search_body(keyValue, &ret)) {
            }
            cursor = reinterpret_cast<node_type*>(ret.val.get());
            if (!stack.push(cursor)) return false;
        }
        return true;

    }

    void clear() {
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
        node_type                 *node = rpGuard.alloc();

        smo_seq = 0;
        init_node(node, true);
        node->clear();
        root    = node;
    }

    void init_node(node_type *node, bool is_leaf) {
        ywBTreeHeader *header = node->get_header();
        node->clear();
        header->is_leaf = is_leaf;
        header->smo_seq = smo_seq;
    }

    volatile uint32_t     smo_seq;
    node_type            *root;
    node_type             nil_node;
    ywRcuPool<node_type>  node_pool;
};

void btree_basic_test();

#endif  // INCLUDE_YWBTREE_H_
