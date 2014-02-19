/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <stdio.h>

#include <ywdlist.h>

void dlist_test() {
    ywDList   head;
    ywDList   slot[16];
    ywDList * iter;
    ywDList * iter2;
    int       i;

    dlist_init(&head);

    YWD_FOREACH(iter, &head)
        printf("%08x ", reinterpret_cast<intptr_t>(iter));
    printf("\n");

    for (i = 0; i < 4; i ++) {
        dlist_attach(&head, &slot[i]);

        YWD_FOREACH(iter2, &head)
            printf("%08x ", reinterpret_cast<intptr_t>(iter2));
        printf("[%d]\n", i);
    }

    YWD_FOREACH(iter, &head) {
        dlist_detach(&head, iter);

        YWD_FOREACH(iter2, &head)
            printf("%08x ", reinterpret_cast<intptr_t>(iter2));
        printf("\n");
    }
}

