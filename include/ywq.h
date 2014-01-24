/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWQ_H_
#define INCLUDE_YWQ_H_

#include <stdint.h>
#include <stdio.h>

struct ywq {
    struct ywq   * prev;
    struct ywq   * next;
    void         * data;
    uint32_t       meta;
    uint32_t       size;
};

#define YWQ_FOREACH(i, h) for ((i) = (h)->next; (i)!= (h); (i) = (i)->next)
#define YWQ_FOREACH_PREV(i, h) for ((i) = (h)->prev; (i)!= (h); (i) = (i)->prev)

void          ywqInit(struct ywq * _head);
void          ywqPush(struct ywq * _head, struct ywq * _target);
struct ywq  * ywqPop(struct ywq * _head);
void          ywqTest();

#endif  // INCLUDE_YWQ_H_
