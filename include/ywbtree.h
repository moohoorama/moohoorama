/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWBTREE_H_
#define INCLUDE_YWBTREE_H_

#include <ywcommon.h>
#include <yworderednode.h>
#include <ywfsnode.h>
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
//    typedef ywFSNode<key_type, PAGE_SIZE, ywBTreeHeader> node_type;

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

            while (!lock_stack.get_last_ptr()->lock()) {
            }
            return true;
        }

     private:
        ywStack<node_type*, MAX_DEPTH>     node_stack;
        ywStack<ywSeqLockGuard, MAX_DEPTH> lock_stack;
    };

    ywBTree() {
//        check_inheritance<ywTypes, KEY>();
//        check_inheritance<ywTypes, VAL>();
        clear();
    }
    ~ywBTree() {
        free_all_node();
        if (!node_pool.all_free()) {
            report();
        }
    }

    void reset() {
        reclaim();
        free_all_node();
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
            if (!split_insert(&rpGuard, &stack, cursor, keyValue)) {
                return false;
            }
        }

        rpGuard.commit();
        return true;
    }

    bool remove(KEY key) {
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
        key_type                   keyValue(key);
        key_type                   ret;
        node_type                 *cursor = find_leaf(keyValue);

        if (cursor->remove(keyValue)) {
            return true;
        }
        return false;
    }
    int32_t get_depth() {
        node_type *cursor = root;
        int32_t    i = 1;

        while (!cursor->get_header()->is_leaf) {
            cursor = reinterpret_cast<node_type*>(
                cursor->get(0).val.get());
            i++;
        }

        return i;
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

        if (!cursor->get_count()) return NULL;

        while (!cursor->search_body(keyValue, &ret)) {
        }

        return ret.val.get();
    }
    void dump() {
        root->dump();
    }

    void report() {
        node_pool.report();
    }


 private:
    node_type *get_child(node_type *node, int32_t idx) {
        return reinterpret_cast<node_type*>(node->get(idx).val.get());
    }
    key_type get_left_most_key(node_type * node) {
        while (!node->get_header()->is_leaf) {
            node = get_child(node, 0);
        }

        return node->get(0);
    }
    bool split_insert(ywRcuPoolGuard<node_type> *rpGuard,
                      ywbtStack                  *stack,
                      node_type                  *node,
                      key_type                    keyValue) {
        /* already be locked last node*/
        node_type      *new_left = rpGuard->alloc();
        node_type      *right    = rpGuard->alloc();
        node_type      *parent;
        int32_t         cur_count = node->get_count();
        int32_t         split_idx = cur_count/2;
        key_type        left_parent_key;
        key_type        right_parent_key;

        if (!new_left || !right) return false;

        assert(node->get_header()->list.is_unlinked());
        assert(rpGuard->free(node));

        init_node(new_left, node->get_header()->is_leaf);
        init_node(right, node->get_header()->is_leaf);
        if (!stack->lock(new_left->get_lock_seq_ptr())) return false;
        if (!stack->lock(right->get_lock_seq_ptr())) return false;

        if (!node->copy_node(new_left, 0, split_idx)) return false;
        if (!node->copy_node(right, split_idx, cur_count)) return false;

        right_parent_key     = get_left_most_key(right);
        right_parent_key.val = ywPtr(right);

        if (0 < right_parent_key.compare(keyValue)) {
            if (!right->insert(keyValue, false)) return false;
        } else {
            if (!new_left->insert(keyValue, false)) return false;
        }

        left_parent_key     = get_left_most_key(new_left);
        left_parent_key.val = ywPtr(new_left);

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
                if (!split_insert(rpGuard, stack, parent, right_parent_key))
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
            assert(cursor);
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
            assert(idx < static_cast<int32_t>(cursor->get_count()));
            if (!stack->push(cursor)) return false;

            if (cursor->get_header()->is_leaf) {
                return true;
            }

            cursor = get_child(cursor, idx);
            assert(cursor);
        }
        return false;
    }

    void free_all_node() {
        {
            ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
            free_node_cascading(&rpGuard, root);
            rpGuard.commit();
        }
        reclaim();
        clear();
    }

    void reclaim() {
        node_pool.aging();
        node_pool.reclaim_to_pool();
    }

    void free_node_cascading(ywRcuPoolGuard<node_type> *rpGuard,
                            node_type                  *node) {
        size_t                    i;

        if (!node->get_header()->is_leaf) {
            for (i = 0; i < node->get_count(); ++i) {
                free_node_cascading(rpGuard, get_child(node, i));
            }
        }

        assert(node->get_header()->list.is_unlinked());
        if (!rpGuard->free(node)) {
            rpGuard->commit();
            rpGuard->free(node);
        }
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
        header->list.init();
    }

    volatile uint32_t     smo_seq;
    node_type            *root;
    node_type             nil_node;
    ywRcuPool<node_type>  node_pool;
};

void btree_basic_test();
void btree_conc_insert(int32_t i);

#endif  // INCLUDE_YWBTREE_H_
