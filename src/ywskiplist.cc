/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywskiplist.h>
#include <gtest/gtest.h>

#include <map>

const int32_t  SK_THREAD_COUNT = 4;
const int32_t  SK_TEST_COUNT = 1024*1024;
ywSkipList     g_skip_list;
typedef struct test_arg_struct {
    int32_t num;
    int32_t op;
} test_arg;


void skiplist_level_test() {
    int32_t   stat[ywSkipList::MAX_LEVEL] = {0};
    int32_t   i;
    int32_t   j;
    int32_t   degree;
    int32_t   val;
    int32_t   expect;

    for (j = 1; j < ywSkipList::MAX_LEVEL; ++j) {
        ywSkipList yw_skip_list(j);

        memset(&stat, 0, sizeof(stat));

        for (i = 0; i < SK_TEST_COUNT; ++i) {
            stat[ yw_skip_list.get_new_level() ]++;
        }
        degree = yw_skip_list.getDegree();
        val = SK_TEST_COUNT;
        for (i = 1; i <= j; ++i) {
            if (i == j || val < degree) {
                break;
            } else {
                expect = val * (degree-1)/degree;
                ASSERT_EQ(expect, stat[i]);
                val -= expect;
            }
        }
    }
}
void skiplist_insert_remove_test() {
    ywSkipList yw_skip_list;
    int32_t    i;

    for (i = 1; i < SK_TEST_COUNT; ++i) {
        ASSERT_TRUE(yw_skip_list.insert(i));
        yw_skip_list.validate();
    }

    for (i = 1; i < SK_TEST_COUNT; ++i) {
        ASSERT_TRUE(yw_skip_list.remove(i));
        yw_skip_list.validate();
    }

    ASSERT_EQ(0, yw_skip_list.get_node_count());
    ASSERT_EQ(0, yw_skip_list.get_key_count());
}

void skiplist_map_insert_perf_test() {
    std::map<int32_t, int32_t> test_map;
    uint32_t                   rnd = 0;
    int32_t                    i;

    for (i = 0; i < SK_TEST_COUNT; ++i) {
        rnd = rand_r(&rnd);
        test_map[rnd]=rnd;
    }
}

void skiplist_insert_perf_test() {
    ywSkipList yw_skip_list;
    uint32_t   rnd = 0;
    int32_t    i;

    for (i = 0; i < SK_TEST_COUNT; ++i) {
        rnd = rand_r(&rnd);
        yw_skip_list.insert(i+1);
    }
}

void skip_test_routine(void * arg) {
    test_arg   *targ = reinterpret_cast<test_arg*>(arg);
    int32_t     i;
    int32_t     count = SK_TEST_COUNT / SK_THREAD_COUNT;

    if (targ->op == 0) {
        for (i = 0; i < count; ++i) {
            ASSERT_TRUE(g_skip_list.insert(i + targ->num * count));
            g_skip_list.validate();
        }
    } else {
        for (i = 0; i < count; ++i) {
            ASSERT_TRUE(g_skip_list.remove(i + targ->num * count));
            g_skip_list.validate();
        }
    }
}

void skiplist_conc_insert_test() {
    test_arg   targ[SK_THREAD_COUNT];
    int32_t    i;

    for (i = 0; i< SK_THREAD_COUNT; ++i) {
        targ[i].num = i+1;
        targ[i].op  = 0;/*insert*/
        ASSERT_TRUE(ywWorkerPool::get_instance()->add_task(
                skip_test_routine, &targ[i]));
    }

    ywWorkerPool::get_instance()->wait_to_idle();

    ASSERT_EQ(SK_TEST_COUNT, g_skip_list.get_key_count());
}

void skiplist_conc_remove_test() {
    test_arg   targ[SK_THREAD_COUNT];
    int32_t    i;

    g_skip_list.validate();
    for (i = 0; i< SK_THREAD_COUNT; ++i) {
        targ[i].num = i+1;
        targ[i].op  = 1;  // remove
        ASSERT_TRUE(ywWorkerPool::get_instance()->add_task(
                skip_test_routine, &targ[i]));
    }

    ywWorkerPool::get_instance()->wait_to_idle();

    ASSERT_EQ(0, g_skip_list.get_node_count());
    ASSERT_EQ(0, g_skip_list.get_key_count());
}

