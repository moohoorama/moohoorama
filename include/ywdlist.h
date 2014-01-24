/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWDLIST_H_
#define INCLUDE_YWDLIST_H_

#include <stdint.h>

#define YWD_FOREACH(i, h) for ((i) = (h)->next; (i) != NULL; (i) = (i)->next)

struct ywDList {
    struct ywDList   * prev;
    struct ywDList   * next;
    void             * data;
    uint32_t           meta;
};

void dlistInit(struct ywDList * _head);
void dlistAttach(struct ywDList * _head, struct ywDList * target);
void dlistDetach(struct ywDList * _head, struct ywDList * target);

void dlistTest();

#endif  // INCLUDE_YWDLIST_H_
