/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <yworderednode.h>
#include <ywworker.h>
#include <ywtypes.h>

typedef int32_t testVar;

static int32_t count = 0;

class ywIntCount {
 public:
    explicit ywIntCount(int32_t src) {
        val = src;
    }
    explicit ywIntCount(Byte *src) {
        read(src);
    }
    void read(Byte *src) {
        memcpy(&val, src, sizeof(val));
    }
    void write(Byte *src) {
        memcpy(src, &val, sizeof(val));
    }
    int32_t   compare(ywIntCount right) {
        return compare(&right);
    }
    int32_t   compare(ywIntCount *right) {
        ++count;
        return right->val - val;
    }
    int32_t   get_size() {
        return sizeof(val);
    }
    void dump() {
        printf("%d ", val);
    }

    int32_t val;
};

template <typename SLOT, size_t PAGE_SIZE>
void OrderedNode_variable_test() {
    typedef
        ywOrderedNode<ywInt, null_test_func, SLOT, PAGE_SIZE>
        test_node;

    test_node  *node = new test_node();
    test_node  *node2 = new test_node();
    size_t     base_size = node->get_page_size() - node->get_meta_size();
    int32_t    slot_size = node->get_slot_size() + sizeof(int32_t);
    int32_t    cnt = base_size / slot_size;
    int32_t    del_cnt = (cnt/2 < 100) ? cnt/2 : 100;
    int32_t    i;

    assert(node->get_free() == base_size);
    for (i = 0; i < cnt; ++i) {
        assert(node->append(ywInt(i)));
    }
    node->sort();
    assert(node->isOrdered());
    assert(node->get_free() == base_size - slot_size*cnt);

    for (i = 0; i < del_cnt; ++i) {
        ywInt ret;
        assert(node->search_body(ywInt(i), &ret));
        assert(i == ret.val);
        assert(node->remove(ywInt(i)));
    }
    node->compact(node2);
    assert(node2->get_free() ==
           base_size - slot_size*(cnt - del_cnt));
    node->clear();

    assert(node->get_free() == base_size);
    for (i = 0; i < cnt; ++i) {
        assert(node->insert(ywInt(i)));
    }
    assert(node->isOrdered());
    assert(node->get_free() == base_size - slot_size*cnt);

    for (i = 0; i < del_cnt; ++i) {
        ywInt ret;
        assert(node->search_body(ywInt(i), &ret));
        assert(i == ret.val);
        assert(node->remove(ywInt(i)));
    }

    delete node;
    delete node2;
}

void OrderedNode_basic_test() {
    assert(8*MB <= get_stack_size() );
    OrderedNode_variable_test<uint8_t, 64>();
    OrderedNode_variable_test<uint8_t, 128>();
    OrderedNode_variable_test<uint8_t, 256>();

    OrderedNode_variable_test<uint16_t,  64>();
    OrderedNode_variable_test<uint16_t,  128>();
    OrderedNode_variable_test<uint16_t,  256>();
    OrderedNode_variable_test<uint16_t,  512>();
    OrderedNode_variable_test<uint16_t,  1*KB>();
    OrderedNode_variable_test<uint16_t,  2*KB>();
    OrderedNode_variable_test<uint16_t,  4*KB>();
    OrderedNode_variable_test<uint16_t,  8*KB>();
    OrderedNode_variable_test<uint16_t, 16*KB>();
    OrderedNode_variable_test<uint16_t, 32*KB>();
    OrderedNode_variable_test<uint16_t, 64*KB>();

    OrderedNode_variable_test<uint32_t,  64>();
    OrderedNode_variable_test<uint32_t,  128>();
    OrderedNode_variable_test<uint32_t,  256>();
    OrderedNode_variable_test<uint32_t,  512>();
    OrderedNode_variable_test<uint32_t,  1*KB>();
    OrderedNode_variable_test<uint32_t,  2*KB>();
    OrderedNode_variable_test<uint32_t,  4*KB>();
    OrderedNode_variable_test<uint32_t,  8*KB>();
    OrderedNode_variable_test<uint32_t, 16*KB>();
    OrderedNode_variable_test<uint32_t, 32*KB>();
    OrderedNode_variable_test<uint32_t, 64*KB>();

    OrderedNode_variable_test<uint32_t, 256*KB>();
    OrderedNode_variable_test<uint32_t, 512*KB>();
    OrderedNode_variable_test<uint32_t, 1*MB>();
    OrderedNode_variable_test<uint32_t, 2*MB>();
}

ywOrderedNode<ywInt, test_binary> *temp_node;

void null_test_func(int32_t) {
}
void test_binary(int32_t seq) {
    int32_t  val;
    int32_t  idx;

    for (val = 0; val < 32; val += 2) {
        ywInt    base(val);
        idx = temp_node->binary_search(&base);
        if (idx < 0) idx = 0;
        assert(0 == base.compare(temp_node->get(idx)));
    }
}

void OrderedNode_bsearch_insert_conc_test() {
    ywOrderedNode<ywInt, test_binary> node;
    int32_t  val;

    for (val = 0; val < 32; val += 2) {
        assert(node.append(ywInt(val)));
    }
    node.sort();

    temp_node = &node;
    for (val = 1; val < 32; val += 2) {
        assert(node.insert(ywInt(val)));
    }

    for (val = 1; val < 32; val += 2) {
        assert(node.remove(ywInt(val)));
    }
}

void OrderedNode_search_test(int32_t cnt, int32_t method) {
    ywOrderedNode<ywIntCount> node;
    int32_t    i;
    int32_t    j;
    int32_t    k;
    int32_t    val = 4;
    int32_t    prev;
    uint32_t   ret;

    prev = count;
    k = 0;
    for (i = 0; i < cnt; ++i) {
        val = (cnt - i)*16;
        assert(node.append(ywIntCount(val)));
        node.sort();
        assert(node.isOrdered());

        for (j = (cnt-i)*16; j <= cnt*16; ++j) {
            ret = node.search(ywIntCount(j));
            assert(0 <= node.get(ret).compare(ywIntCount(j)));
            ++k;
        }
    }
    printf("try count     : %d\n", k);
    printf("compare count : %d\n", count-prev);
    printf("compare cost  : %.3f\n", (count-prev)*1.0f/k);
}

class ywOrderStressTestClass {
 public:
    static const int32_t TEST_COUNT = 1024*32;

    explicit ywOrderStressTestClass() {
    }

    void run();

    int32_t               operation;
    ywOrderedNode<ywInt> *node;
};

void ywOrderStressTestClass::run() {
    int32_t   val;
    ywInt     ret;
    int32_t   i;
    int32_t   try_count = 0;

    for (i = 0; i < TEST_COUNT; ++i) {
        if (operation == 0) {
            for (val = 1; val < 256; val += 2) {
                node->insert(ywInt(val));
            }
            for (val = 1; val < 256; val += 2) {
                node->remove(ywInt(val));
            }
        } else {
            for (val = 0; val < 256; val += 2) {
                do {
                    try_count++;
                } while (!node->search_body(ywInt(val), &ret));

                if (ret.compare(ywInt(val))) {
                    ret.dump();
                    ywInt(val).dump();
                    assert(false);
                }
            }
        }
    }
    printf("[%6d] %10d %10.2f\n",
           operation, try_count, 128*TEST_COUNT*100.0/try_count);
}

void onode_stress_task(void *arg) {
    ywOrderStressTestClass *tc = reinterpret_cast<ywOrderStressTestClass*>(arg);

    tc->run();
}


void OrderedNode_stress_test(int32_t thread_count) {
    ywWorkerPool                      *tpool = ywWorkerPool::get_instance();
    ywOrderStressTestClass             tc[MAX_THREAD_COUNT];
    ywOrderedNode<ywInt>               node;
    int32_t                            val;
    int32_t                            i;

    node.clear();

    for (val = 0; val < 256; val += 2) {
        assert(node.insert(ywInt(val)));
    }
    node.sort();

    printf("%8s %10s %10s\n", "THREAD", "TRY_COUNT", "SUCCESS_RATE");
    for (i = 0; i < thread_count; ++i) {
        tc[i].operation = i;
        tc[i].node      = &node;
        assert(tpool->add_task(onode_stress_task, &tc[i]));
    }
    tpool->wait_to_idle();
}
