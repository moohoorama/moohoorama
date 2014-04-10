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
    static const int32_t MAX_DEPTH = 16;

    typedef ywKey<KEY, VAL> key_type;
    typedef ywOrderedNode<key_type, null_test_func,
            uint16_t, PAGE_SIZE, ywBTreeHeader> node_type;

    class ywbtStack {
     public:
        explicit ywbtStack() {
        }
        ~ywbtStack() {
        }

        bool push(node_type *node) {
            return node_stack.push(node);
        }

        bool pop(node_type **node) {
            return node_stack.pop(node);
        }

        bool lock(volatile uint32_t *seq) {
            if (!lock_stack.push(ywSeqLockGuard(seq))) return false;

            lock_stack.get_last_ptr()->lock();
            return true;
        }

     private:
        ywStack<node_type*, MAX_DEPTH>     node_stack;
        ywStack<ywSeqLockGuard, MAX_DEPTH> lock_stack;
    };

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

        if (!cursor->insert(keyValue)) {
            ywSeqLockGuard  smo_guard(&smo_seq);
            ywbtStack       stack;

            if (!smo_guard.lock()) return false;
            if (!traverse_with_stack(keyValue, &stack)) return false;
            if (!stack.pop(&cursor)) return false;
            if (!split_Insert(&rpGuard, &stack, cursor, keyValue)) {
                return false;
            }
        }

        rpGuard.commit();
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
        node_type                 *cursor = find_leaf(keyValue);

        while (!cursor->search_body(keyValue, &ret)) {
        }

        return ret.val.get();
    }
    void dump() {
        root->dump();
    }

 private:
    bool split_Insert(ywRcuPoolGuard<node_type> *rpGuard,
                      ywbtStack                  *stack,
                      node_type                  *node,
                      key_type                    keyValue) {
        /* already be locked last node*/
        node_type      *new_left = rpGuard->alloc();
        node_type      *right    = rpGuard->alloc();
        node_type      *parent;
        int32_t         cur_count = node->get_count();
        key_type        left_parent_key;
        key_type        right_parent_key;

        if (!new_left || !right) return false;

        init_node(new_left, node->get_header()->is_leaf);
        init_node(right, node->get_header()->is_leaf);
        if (!stack->lock(new_left->get_lock_seq_ptr())) return false;
        if (!stack->lock(right->get_lock_seq_ptr())) return false;

        if (!node->copy_node(new_left, 0, cur_count/2)) return false;
        if (!node->copy_node(right, cur_count/2, cur_count)) return false;

        left_parent_key     = new_left->get(0);
        left_parent_key.val = ywPtr(new_left);

        right_parent_key     = right->get(0);
        right_parent_key.val = ywPtr(right);

        if (!new_left->insert(keyValue, false)) return false;
        if (!stack->pop(&parent)) { /* stack is root */
            /*make new root*/
            parent = rpGuard->alloc();

            if (!parent) return false;

            init_node(parent, false /*is_leaf*/);
            if (!parent->insert(right_parent_key)) return false;
            if (!parent->insert(left_parent_key)) return false;

            root    = parent;
        } else {
            if (!parent->inplace_update(left_parent_key)) return false;
            if (!parent->insert(right_parent_key)) {
                if (!stack->lock(parent->get_lock_seq_ptr())) return false;
                if (!split_Insert(rpGuard, stack, parent, right_parent_key))
                    return false;
            }
        }

        return true;
    }

    node_type *find_leaf(key_type keyValue) {
        key_type   ret;
        node_type *cursor = root;
        uint32_t   cur_smo_seq = smo_seq;

        while (!cursor->get_header()->is_leaf) {
            while (!cursor->search_body(keyValue, &ret)) {
            }
            if (cursor->get_header()->smo_seq < cur_smo_seq) {
                cursor = reinterpret_cast<node_type*>(ret.val.get());
            } else {
                cursor = root; /*retry*/
                cur_smo_seq = smo_seq;
            }
        }
        return cursor;
    }

    bool traverse_with_stack(key_type keyValue, ywbtStack *stack) {
        key_type   ret;
        node_type *cursor = root;
        int32_t    idx;
        int32_t    i;

        for (i = 0; i < MAX_DEPTH; ++i) {
            if (cursor->get_header()->is_leaf) {
                if (!stack->lock(cursor->get_lock_seq_ptr())) return false;
            }
            idx = cursor->binary_search(keyValue);
            if (idx < 0 ) idx = 0;
            if (!stack->push(cursor)) return false;

            if (cursor->get_header()->is_leaf) {
                return true;
            }

            cursor = reinterpret_cast<node_type*>(cursor->get(idx).val.get());
        }
        return false;
    }

    void clear() {
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
        node_type                 *node = rpGuard.alloc();

        assert(node);

        smo_seq = 0;
        init_node(node, true);
        root    = node;

        rpGuard.commit();
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
