/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSLL_H_
#define INCLUDE_YWSLL_H_

#include <ywcommon.h>

/* Single Linked List */
typedef struct ywsllNodeStruct ywsllNode;

struct ywsllNodeStruct {
    ywsllNodeStruct():next(this) {
    }
    ywsllNode * next;
};

inline void ywsll_init(ywsllNode *head) {
    head->next = head;
}
inline void ywsll_attach(ywsllNode *prev, ywsllNode *new_node) {
    new_node->next = prev->next;
    prev->next     = new_node;
}
inline void ywsll_detach(ywsllNode *prev) {
    prev->next = prev->next->next;
}
inline ywsllNode *ywsll_pop(ywsllNode *head) {
    ywsllNode *ret = head->next;
    if (ret == head) {
        return NULL;
    }
    head->next = ret->next;
    return ret;
}

#endif  // INCLUDE_YWSLL_H_
