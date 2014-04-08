/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWBTREE_H_
#define INCLUDE_YWBTREE_H_

#include <ywcommon.h>
#include <yworderednode.h>
#include <ywsl.h>
#include <ywmempool.h>
#include <ywrcupool.h>

class ywBTreeHeader {
 public:
    ywDList   list;
    uint64_t  smo_seq;
};

template<typename KEY, typename VAL, size_t PAGE_SIZE = 32*KB>
class ywBTree {
 public:
    static const size_t  PID_SIZE = sizeof(void*);

    static int32_t get_size(Byte * ptr) {
        return key_size(ptr) + PID_SIZE;
    }

//    typedef ywKey<key_comp, key_size, val_size> key_type;
    typedef ywOrderedNode<key_comp, key_size, null_test_func,
            uint16_t, PAGE_SIZE, ywBTreeHeader> node_type;

    ywBTree() {
        clear();
    }

    bool insert(Byte *key, Byte *data) {
//        ywSeqLockGuard guard(&smo_seq);
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
//        key_type                   key(key, data);

//        if (root->insert(

        return true;
    }
    bool remove(Byte *key) {
        return true;
    }
    Byte *find(Byte *key) {
        return NULL;
    }

 private:
    void clear() {
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);
        node_type                 *node = rpGuard.alloc();

        smo_seq = 0;
        node->clear();
        root    = node;
    }

    volatile uint32_t     smo_seq;
    node_type            *root;
    node_type             nil_node;
    ywRcuPool<node_type>  node_pool;
};

void btree_basic_test();

#endif  // INCLUDE_YWBTREE_H_
