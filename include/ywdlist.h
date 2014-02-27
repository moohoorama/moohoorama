/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWDLIST_H_
#define INCLUDE_YWDLIST_H_

#include <ywcommon.h>

#define YWD_FOREACH(i, h) for ((i) = (h)->next; (i) != (h); (i) = (i)->next)
#define YWDL_LINK(c, n) (c)->next = (n); (n)->prev=(c);

typedef struct ywDListStruct ywdl_t;

struct ywDListStruct {
    ywDListStruct():prev(this), next(this) {
    }

    ywdl_t   * prev;
    ywdl_t   * next;
};

inline void ywdl_init(ywdl_t * _head) {
    _head->prev = _head;
    _head->next = _head;
}

inline void ywdl_attach(ywdl_t * _head, ywdl_t * target) {
    YWDL_LINK(target, _head->next);
    YWDL_LINK(_head, target);
}

inline void ywdl_detach(ywdl_t * target) {
    YWDL_LINK(target->prev, target->next);
}

inline ywdl_t *ywdl_pop(ywdl_t * _head) {
    ywdl_t * target = _head->next;
    if (target == _head) {
        return NULL;
    }
    ywdl_detach(target);

    return target;
}

inline void ywdl_move(ywdl_t *target_head, ywdl_t *target_tail, ywdl_t *dst) {
    ywdl_t * src_remain_tail = target_head->prev;
    ywdl_t * src_remain_head = target_tail->next;
    YWDL_LINK(target_tail, dst->next);
    YWDL_LINK(dst, target_head);
    YWDL_LINK(src_remain_tail, src_remain_head);
}

inline void ywdl_move_all(ywdl_t *src, ywdl_t *dst) {
    ywdl_t * target_head = src->next;
    ywdl_t * target_tail = src->prev;
    if (target_head == src) { /* empty src */
        return;
    }

    ywdl_move(target_head, target_tail, dst);
}

inline void ywdl_move_n(ywdl_t *src, ywdl_t *dst, int32_t count) {
    ywdl_t   *head = src->next;
    ywdl_t   *tail;
    int32_t   i;

    if (head == src) { /* empty src */
        return;
    }

    for (i = 0, tail = src; i < count; i++, tail = tail->next) {
    }
    ywdl_move(head, tail, dst);
}

inline int32_t ywdl_stupid_get_count(ywdl_t * _head) {
    ywdl_t * iter;
    int      count = 0;
    YWD_FOREACH(iter, _head) {
        count++;
    }
    return count;
}

void ywdl_test();

#endif  // INCLUDE_YWDLIST_H_
