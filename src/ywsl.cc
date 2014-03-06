/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywsl.h>
#include <stdio.h>

bool ywSyncList::add(ywNode *node) {
    if (!head.lock.WLock()) {
        return false;
    }

    if (!node->lock.WLock()) {
        head.lock.release();
        return false;
    }

    node->prev = &head;
    node->next = head.next;

    node->next->prev = node;
    node->prev->next = node;

    node->lock.release();
    head.lock.release();

    __sync_fetch_and_add(&count, 1);

    return true;
}

bool ywSyncList::remove(ywNode *node) {
    ywNode *prev = node->prev;

    if (!prev->lock.WLock()) {
        return false;
    }
    if (prev != node->prev || !node->lock.WLock()) {
        prev->lock.release();
        return false;
    }

    prev->next = node->next;
    node->next->prev = prev;

    node->lock.release();
    prev->lock.release();

    __sync_fetch_and_sub(&count, 1);

    return true;
}


void ywSyncList::print() {
    printf("%d:\n", static_cast<int>(count));
    for (ywNode *iter = begin(); iter != end(); iter = iter->next) {
        printf("%"PRIxPTR" -> %"PRIxPTR"\n",
                reinterpret_cast<intptr_t>(iter),
                reinterpret_cast<intptr_t>(iter->data));
    }
    printf("\n");
}
