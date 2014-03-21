/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <yworderednode.h>

typedef int32_t testVar;
void OrderedNode_basic_test() {
    ywOrderedNode<> node;
    int32_t         val = 4;

    node.dump();
    val = 5; assert(node.insert(&val));
    val = 8; assert(node.insert(&val));
    val = 2; assert(node.insert(&val));
    val =14; assert(node.insert(&val));
    val =21; assert(node.insert(&val));
    val =11; assert(node.insert(&val));
    node.dump();
}
