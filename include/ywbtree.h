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
    static const size_t  PID_SIZE = sizeof(void*);

    typedef ywKey<KEY, VAL> key_type;
    typedef ywOrderedNode<key_type, null_test_func,
            uint16_t, PAGE_SIZE, ywBTreeHeader> node_type;

    ywBTree() {
        check_inheritance<ywTypes, KEY>();
        check_inheritance<ywTypes, VAL>();
        clear();
    }

    bool insert(KEY key, void *value) {
        return insert(key, ywPtr(value));
    }

    bool insert(KEY key, VAL value) {
//        ywSeqLockGuard guard(&smo_seq);
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
        key_type                   keyValue(key, value);

        assert(root->insert(keyValue));
//        if (root->insert(

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
        node_type                 *cursor = root;

        while (!cursor->get_header()->is_leaf) {
            while (!cursor->search_body(keyValue, &ret)) {
            }
            cursor = reinterpret_cast<node_type*>(ret.val.get());
        }

        while (!cursor->search_body(keyValue, &ret)) {
        }

        return ret.val.get();
    }
    void dump() {
        root->dump();
    }

 private:
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
