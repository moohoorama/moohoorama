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

template<compFunc comp, sizeFunc _get_size, size_t PAGE_SIZE = 32*KB>
class ywBTree {
 public:
    static const size_t  PID_SIZE = sizeof(void*);

    static int32_t get_size(char * ptr) {
        return _get_size(ptr) + PID_SIZE;
    }

    typedef ywOrderedNode<comp, _get_size, null_test_func,
            uint16_t, PAGE_SIZE, ywBTreeHeader> node_type;

    ywBTree() {
        clear();
    }

    bool insert(char *key, char *data) {
//        ywSeqLockGuard guard(&smo_seq);
        ywRcuPoolGuard<node_type>  rpGuard(&node_pool);

        return true;
    }
    bool remove(char *key) {
        return true;
    }
    char *find(char *key) {
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

char *bt_find(void *bt, char *key);

void btree_basic_test();

#endif  // INCLUDE_YWBTREE_H_
