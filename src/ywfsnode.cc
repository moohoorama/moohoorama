/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywfsnode.h>
#include <ywworker.h>
#include <ywtypes.h>

void FSNode_basic_test() {
    typedef ywFSNode<ywInt> node_type;
    node_type  node, node2;
    size_t     base_size = node.get_page_size() - node.get_meta_size();
    int32_t    slot_size = node.get_slot_size();
    int32_t    cnt = base_size / slot_size;
    int32_t    del_cnt = (cnt/2 < 100) ? cnt/2 : 100;
    int32_t    i;

    assert(node.get_free() == base_size);
    for (i = 0; i < cnt; ++i) {
        assert(node.append(ywInt(i)));
    }
    node.sort();
    assert(node.isOrdered());
    assert(node.get_free() == base_size - slot_size*cnt);

    for (i = 0; i < del_cnt; ++i) {
        ywInt ret;
        assert(node.search_body(ywInt(i), &ret));
        assert(i == ret.val);
        assert(node.remove(ywInt(i)));
    }
    node.compact(&node2);
    assert(node2.get_free() ==
           base_size - slot_size*(cnt - del_cnt));
    node.clear();

    assert(node.get_free() == base_size);
    for (i = 0; i < cnt; ++i) {
        assert(node.insert(ywInt(i)));
        assert(node.isOrdered());
    }
    assert(node.isOrdered());
    assert(node.get_free() == base_size - slot_size*cnt);

    for (i = 0; i < del_cnt; ++i) {
        ywInt ret;
        assert(node.search_body(ywInt(i), &ret));
        assert(i == ret.val);
        assert(node.remove(ywInt(i)));
    }
}
