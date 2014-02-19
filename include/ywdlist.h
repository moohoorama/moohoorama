/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWDLIST_H_
#define INCLUDE_YWDLIST_H_

#include <ywcommon.h>

#define YWD_FOREACH(i, h) for ((i) = (h)->next; (i) != NULL; (i) = (i)->next)
#define YWDL_LINK(c, n) (c)->next = (n); (n)->prev=(c);

typedef struct ywDListStruct ywDList;

struct ywDListStruct {
    ywDList   * prev;
    ywDList   * next;
};

inline void dlist_init(ywDList * _head) {
    _head->prev = _head;
    _head->next = _head;
}

inline void dlist_attach(ywDList * _head, ywDList * target) {
    YWDL_LINK(target, _head->next);
    YWDL_LINK(_head, target);
}

inline void dlist_detach(ywDList * _head, ywDList * target) {
    YWDL_LINK(target->prev, target->next);
}

void dlist_test();

#endif  // INCLUDE_YWDLIST_H_
