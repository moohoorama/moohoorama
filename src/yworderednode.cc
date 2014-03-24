/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <yworderednode.h>

typedef int32_t testVar;

static int32_t count = 0;
int32_t intComp(char *_left, char *_right) {
    int32_t *left = reinterpret_cast<int32_t*>(_left);
    int32_t *right= reinterpret_cast<int32_t*>(_right);

    return *right - *left;
}

int32_t intCompCount(char *_left, char *_right) {
    int32_t *left = reinterpret_cast<int32_t*>(_left);
    int32_t *right= reinterpret_cast<int32_t*>(_right);

    ++count;

    return *right - *left;
}


void OrderedNode_basic_test(int32_t cnt, int32_t method) {
    ywOrderedNode<> node;
    int32_t         val = 4;
    int32_t         i;
    int32_t         j;
    int32_t         k;
    int32_t         prev;
    uint32_t        ret;

    val = 5; assert(node.insert(&val));
    val = 8; assert(node.insert(&val));
    val = 2; assert(node.insert(&val));
    val =14; assert(node.insert(&val));
    val =21; assert(node.insert(&val));
    val =11; assert(node.insert(&val));
    node.dump();
    node.sort<intComp>();
    assert(node.isOrdered<intComp>());
    node.dump();

    prev = count;
    node.clear();
    k = 0;
    for (i = 0; i < cnt; ++i) {
        val = (cnt - i)*16;
        assert(node.insert(&val));
        node.sort<intComp>();
        assert(node.isOrdered<intComp>());

        for (j = (cnt-i)*16; j <= cnt*16; ++j) {
            if (method == 0) {
                ret = node.binary_search<intCompCount>(
                    reinterpret_cast<char*>(&j));
            } else {
                ret = node.interpolation_search<intCompCount>
                    (reinterpret_cast<char*>(&j));
            }
            assert(0 <=
                   intComp(node.get_slot(ret), reinterpret_cast<char*>(&j)));
            ++k;
        }
    }
    printf("try count     : %d\n", k);
    printf("compare count : %d\n", count-prev);
    printf("compare cost  : %.3f\n", (count-prev)*1.0f/k);
}
