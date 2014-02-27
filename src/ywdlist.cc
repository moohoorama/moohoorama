/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <stdio.h>
#include <ywdlist.h>
#include <gtest/gtest.h>

void ywdl_test() {
    ywdl_t   head;
    ywdl_t   head_noinit;
    ywdl_t   slot[16];
    ywdl_t * iter;
    int      i;

    ywdl_init(&head);
    ASSERT_EQ(0, ywdl_stupid_get_count(&head));
    ASSERT_EQ(0, ywdl_stupid_get_count(&head_noinit));

    for (i = 0; i < 4; i ++) {
        ASSERT_EQ(i, ywdl_stupid_get_count(&head));
        ywdl_attach(&head, &slot[i]);
        ASSERT_EQ(i+1, ywdl_stupid_get_count(&head));
    }

    i = 4;
    YWD_FOREACH(iter, &head) {
        ASSERT_EQ(i, ywdl_stupid_get_count(&head));
        ywdl_detach(iter);
        ASSERT_EQ(--i, ywdl_stupid_get_count(&head));
    }

    ASSERT_EQ(0, ywdl_stupid_get_count(&head));
    ASSERT_EQ(0, ywdl_stupid_get_count(&head_noinit));

    for (i = 0; i < 4; i ++) {
        ywdl_attach(&head, &slot[i]);
    }
    for (i = 0; i < 4; i ++) {
        ywdl_attach(&head_noinit, &slot[i+4]);
    }
    ASSERT_EQ(4, ywdl_stupid_get_count(&head));
    ASSERT_EQ(4, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_all(&head, &head_noinit);
    ASSERT_EQ(0, ywdl_stupid_get_count(&head));
    ASSERT_EQ(8, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_all(&head, &head_noinit);
    ASSERT_EQ(0, ywdl_stupid_get_count(&head));
    ASSERT_EQ(8, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_n(&head_noinit, &head, 2);
    ASSERT_EQ(2, ywdl_stupid_get_count(&head));
    ASSERT_EQ(6, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_n(&head_noinit, &head, 2);
    ASSERT_EQ(4, ywdl_stupid_get_count(&head));
    ASSERT_EQ(4, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_n(&head_noinit, &head, 2);
    ASSERT_EQ(6, ywdl_stupid_get_count(&head));
    ASSERT_EQ(2, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_n(&head_noinit, &head, 2);
    ASSERT_EQ(8, ywdl_stupid_get_count(&head));
    ASSERT_EQ(0, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_all(&head_noinit, &head);
    ASSERT_EQ(8, ywdl_stupid_get_count(&head));
    ASSERT_EQ(0, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_all(&head, &head_noinit);
    ASSERT_EQ(0, ywdl_stupid_get_count(&head));
    ASSERT_EQ(8, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_all(&head_noinit, &head);
    ASSERT_EQ(8, ywdl_stupid_get_count(&head));
    ASSERT_EQ(0, ywdl_stupid_get_count(&head_noinit));

    ywdl_move_all(&head_noinit, &head);
    ASSERT_EQ(8, ywdl_stupid_get_count(&head));
    ASSERT_EQ(0, ywdl_stupid_get_count(&head_noinit));
}

