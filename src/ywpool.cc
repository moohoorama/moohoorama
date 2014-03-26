/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywpool.h>
#include <stdio.h>
#include <gtest/gtest.h>

class test_type {
 public:
    int32_t val[4];
};

void ywPoolTest() {
    ywPool<test_type> pdlist;
    test_type                  slots[64];
    int32_t                    i;

    for (i = 0; i < 32; ++i) pdlist.push(&slots[0+i]);
    pdlist.print();

    for (i = 0; i < 16; ++i) pdlist.push(&slots[32+i]);
    pdlist.print();

    for (i = 0; i < 16; ++i) pdlist.push(&slots[48+i]);
    pdlist.print();

    for (i = 0; i < 16; ++i) assert(pdlist.pop());
    pdlist.print();

    for (i = 0; i < 8; ++i) assert(pdlist.pop());
    pdlist.print();

    for (i = 0; i < 8; ++i) assert(pdlist.pop());
    pdlist.print();

    for (i = 0; i < 32; ++i) assert(pdlist.pop());
    pdlist.print();
}

class ywPDListTest {
 public:
  static const int32_t   SLOT_COUNT = 1024;
  static const int32_t   TRY_COUNT  = 1024*4;
  ywPool<test_type>     *pdlist;
  test_type              slot[SLOT_COUNT];

  void run();
};

void pdlist_run_task(void *arg) {
    ywPDListTest *tc = reinterpret_cast<ywPDListTest*>(arg);

    tc->run();
}

void ywPDListTest::run() {
    int32_t      i;
    int32_t      j;
    int32_t      count;
    test_type   *ptr[SLOT_COUNT];

    for (i = 0; i < SLOT_COUNT; ++i) {
        pdlist->push(&slot[i]);
    }
    for (j = 0; j < TRY_COUNT; ++j) {
        count = 0;
        for (i = 0; i < SLOT_COUNT; ++i) {
            ptr[count] = pdlist->pop();
            if (ptr[count]) {
                ++count;
            }
        }
        for (i = 0; i < count; ++i) {
            pdlist->push(ptr[i]);
        }
    }

    do {
        ptr[0] = pdlist->pop();
    } while (ptr[0]);
}

void ywPoolCCTest(int32_t num) {
    ywThreadPool      *tpool = ywThreadPool::get_instance();
    ywPool<test_type>  pdlist;
    ywPDListTest       tc[MAX_THREAD_COUNT];
    int32_t            i;

    assert(num < MAX_THREAD_COUNT);

    for (i = 0; i < num; ++i) {
        tc[i].pdlist    = &pdlist;
    }

    for (i = 0; i < num; ++i) {
//        pdlist_run_task(&tc[i]);
        EXPECT_TRUE(tpool->add_task(pdlist_run_task, &tc[i]));
    }

    tpool->wait_to_idle();
}

