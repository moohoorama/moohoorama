/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#ifndef INCLUDE_YWSL_H_
#define INCLUDE_YWSL_H_

#include <ywcommon.h>

class ywDList {
 public:
    explicit ywDList():next(this), prev(this) {
    }

    void init() {
        next = this;
        prev = this;
    }

    void link(ywDList * target) {
        next         = target;
        target->prev = this;
    }

    bool is_unlinked() {
        return (next == this) && (prev == this);
    }

    void attach(ywDList * target) {
        target->link(next);
        this->link(target);
    }

    void detach() {
        prev->link(next);
    }

    ywDList * pop() {
        ywDList * ret = next;
        if (ret == this) {
            return NULL;
        }
        next->detach();

        return ret;
    }

    void bring(ywDList * list) {
        bring(list->next, list->prev);
    }
    void bring(ywDList * head, ywDList * tail) {
        ywDList * before_head = head->prev;
        ywDList * after_tail = tail->next;

        /*attach*/
        tail->link(next);
        this->link(head);

        /*detach*/
        before_head->link(after_tail);
    }

    ywDList    * next;
    ywDList    * prev;
};

#endif  // INCLUDE_YWSL_H_

