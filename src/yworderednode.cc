/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <yworderednode.h>

typedef int32_t testVar;

int32_t intComp(char *_left, char *_right) {
    int32_t *left = reinterpret_cast<int32_t*>(_left);
    int32_t *right= reinterpret_cast<int32_t*>(_right);

    return *right - *left;
//    return memcmp(_right, _left, 4);
}

void OrderedNode_basic_test(int32_t cnt, int32_t method) {
    ywOrderedNode<> node;
    int32_t         val = 4;
    int32_t         i;
    int32_t         j;
    uint32_t        ret;

//    node.dump();
    val = 5; assert(node.insert(&val));
    val = 8; assert(node.insert(&val));
    val = 2; assert(node.insert(&val));
    val =14; assert(node.insert(&val));
    val =21; assert(node.insert(&val));
    val =11; assert(node.insert(&val));
//    node.dump();
    node.sort<intComp>();
    assert(node.isOrdered<intComp>());
//    node.dump();

    node.clear();
    for (i = 0; i < cnt; ++i) {
        val = (cnt - i)*4;
        assert(node.insert(&val));
        node.sort<intComp>();
        assert(node.isOrdered<intComp>());

        for (j = (cnt-i)*4; j <= cnt*4; ++j) {
            if (method == 0) {
                ret = node.binary_search<intComp>(reinterpret_cast<char*>(&j));
            } else {
                ret = node.interpolation_search<intComp>
                    (reinterpret_cast<char*>(&j));
            }
            val = *reinterpret_cast<int*>(node.get_slot(ret));
            assert(val <= j);
        }
    }
//    node.dump();
}
