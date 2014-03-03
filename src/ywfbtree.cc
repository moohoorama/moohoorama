/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywfbtree.h>
#include <ywthread.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>
#include <ywrcuref.h>
#include <ywmempool.h>

static const int32_t FB_SLOT_MAX       = 128;
static const int32_t FB_LAST_SLOT      = FB_SLOT_MAX-1;
static const int32_t FB_5_5_SPLIT      = FB_SLOT_MAX/2;
static const int32_t FB_1_9_SPLIT      = 1;
static const int32_t FB_9_1_SPLIT      = FB_LAST_SLOT;
static const fbKey   FB_NIL_KEY        = UINT32_MAX;
static const fbKey   FB_LEFT_MOST_KEY  = 0;
static const int32_t FB_DEPTH_MAX      = 10;

typedef struct       fbNodeStruct   fbn_t;
typedef struct       fbRootStruct   fbr_t;
typedef struct       fbTreeStruct   fbt_t;
typedef struct       fbStackStruct  fbs_t;
typedef struct       fbCursorStruct fbc_t;

/*********** Declare Functions ***********/
inline fbt_t *fb_get_handle(void *_fbt);

inline fbn_t *fb_create_node();
inline void   fb_free_node(fbn_t *fbn);

inline void   fb_reset_node(fbn_t *fnb, int32_t begin_idx = 0);
inline void   fb_insert_into_node(fbn_t *fbn, fbs_t *fbs,
                                  fbKey key, fbn_t * child);
inline void   fb_remove_in_node(fbn_t *fbn, int32_t idx);
inline void   fb_add_stat(fbt_t *fbt, fbs_t *fbs);

/*********** Concurrency functions ***********/

inline bool   fb_w_lock(fbn_t **node);
inline void   fb_w_unlock(fbn_t **node);
inline bool   fb_r_lock(fbn_t **node);
inline void   fb_r_unlock(fbn_t **node);

/**************** SMO(Structure Modify Operation) *****************/
bool fb_split_node(fbt_t *fbt, fbn_t *fbn, fbs_t *fbs);

inline int32_t  fb_search_in_node(fbn_t *fbn, fbKey key);
inline fbn_t   *fb_traverse(fbt_t *fbt, fbKey key, fbs_t *fbs);
inline bool     fb_is_full_node(fbn_t *fbn);
inline bool     fb_is_empty_node(fbn_t *fbn);

inline void     fb_dump_node(int height, fbn_t *fbn);
inline void     fb_validation_node(int height, fbn_t *fbn);

/************* DataStructure **********/
struct fbNodeStruct {
    explicit fbNodeStruct() {
        fb_reset_node(this);
    }
    fbKey   key[FB_SLOT_MAX];
    fbn_t  *child[FB_SLOT_MAX];
};
fbn_t  fb_nil_node_instance;
fbn_t *fb_nil_node=&fb_nil_node_instance;

/*
struct fbRootStruct {
    fbn_t          *root;
    int32_t         level;
    fbTraverseFunc  traverse;
};
*/

struct fbTreeStruct {
    fbTreeStruct();

    fbn_t                 *root;
    int32_t                level;

    ywAccumulator<int64_t> ikey_count;
    ywAccumulator<int64_t> key_count;
    ywAccumulator<int64_t> node_count;
};
fbt_t  fb_nil_tree_instance;
fbt_t *fb_nil_tree=&fb_nil_tree_instance;

fbTreeStruct::fbTreeStruct() {
    if (this == &fb_nil_tree_instance) {
        root = fb_nil_node;
    }
}

struct fbCursorStruct {
    fbn_t   *node;
    int32_t  idx;
};

struct fbStackStruct {
    fbStackStruct():
        ikey_count(0), key_count(0), node_count(0) {
    }
    fbc_t   cursor[FB_DEPTH_MAX];
    int32_t depth;
    int32_t ikey_count;
    int32_t key_count;
    int32_t node_count;
};


ywMemPool<fbn_t>             fb_node_pool;
ywMemPool<fbt_t>             fbt_tree_pool;

void *fb_create() {
    fbt_t * fbt = fbt_tree_pool.alloc();

    if (!fbt) {
        return fb_nil_tree;
    }

    new (fbt) fbt_t();

    fbt->root       = fb_nil_node;
    fbt->level      = 0;
    fbt->ikey_count.reset();
    fbt->key_count.reset();
    fbt->node_count.reset();

    return reinterpret_cast<void*>(fbt);
}

bool  fb_insert(void *_fbt, fbKey key, void *_data) {
    fbt_t   *fbt = fb_get_handle(_fbt);
    fbn_t   *data = reinterpret_cast<fbn_t*>(_data);
    fbn_t   *fbn;
    fbs_t    fbs;

    if (!fb_w_lock(&fbt->root)) {
        return false;
    }

    if (fbt->level) {
        fbn = fb_traverse(fbt, key, &fbs);
    } else {
        if (!(fbn = fb_create_node())) {
            fb_w_unlock(&fbt->root);
            return false;
        }
        fb_reset_node(fbn);
        fbt->root = fbn;
        ++fbt->level;

        fbs.depth = 0;
        fbs.cursor[0].node = fbn;
        fbs.cursor[0].idx  = -1;
        ++fbs.node_count;
    }

    if ((fbs.cursor[ fbs.depth ].idx != -1) &&
        (fbn->key[ fbs.cursor[ fbs.depth ].idx ] == key)) { /* dupkey */
        fb_w_unlock(&fbt->root);
        return false;
    }
    fb_insert_into_node(fbn, &fbs, key, data);

    if (fb_is_full_node(fbn)) {
        if (!fb_split_node(fbt, fbn, &fbs)) {
            fb_w_unlock(&fbt->root);
            return false;
        }
    }
    fbs.key_count = +1;
    fb_add_stat(fbt, &fbs);

    fb_w_unlock(&fbt->root);

    return true;
}

bool  fb_remove(void *_fbt, fbKey key) {
    fbt_t *fbt = fb_get_handle(_fbt);
    fbn_t *fbn;
    fbs_t  fbs;
    fbc_t *cursor;

    if (!fb_w_lock(&fbt->root)) {
        return false;
    }

    if (fbt->root == fb_nil_node) {
        fb_w_unlock(&fbt->root);
        return false;
    }

    fbn = fb_traverse(fbt, key, &fbs);
    assert(fbn != fb_nil_node);

    cursor = &fbs.cursor[ fbs.depth ];
    assert(fbn == cursor->node);

    if (cursor->node->key[ cursor->idx] != key) {
        fb_w_unlock(&fbt->root);
        return false;
    }

    fb_remove_in_node(cursor->node, cursor->idx);

    while (fb_is_empty_node(cursor->node)) {
        --fbs.node_count;
        fb_free_node(cursor->node);
        if (fbs.depth == 0) { /* remove root*/
            fbt->root  = fb_nil_node;
            fbt->level = 0;
            break;
        }
        --fbs.depth;
        cursor = &fbs.cursor[ fbs.depth ];
        --fbs.ikey_count;
        fb_remove_in_node(cursor->node, cursor->idx);
        if (cursor->node->key[0] != FB_NIL_KEY) {
            cursor->node->key[0] = FB_LEFT_MOST_KEY;
        }
    }

    fbs.key_count = -1;
    fb_add_stat(fbt, &fbs);
    fb_w_unlock(&fbt->root);
    return true;
}

void *fb_find(void *_fbt, fbKey key) {
    fbt_t   *fbt = fb_get_handle(_fbt);
    fbn_t   *fbn = fbt->root;
    int32_t  idx;
    int32_t  i = fbt->level;

    while (!fb_r_lock(&fbt->root)) {
    }

    while (i--) {
        idx = fb_search_in_node(fbn, key);
        fbn = fbn->child[idx];
    }

    fb_r_unlock(&fbt->root);

    return fbn;
}

void  fb_dump(void *_fbt) {
    fbt_t   *fbt = fb_get_handle(_fbt);

    printf("==================================\n");
    fb_dump_node(fbt->level, fbt->root);
}

void fb_validation(void *_fbt) {
    fbt_t   *fbt = fb_get_handle(_fbt);

    fb_validation_node(fbt->level, fbt->root);
}

void fb_report(void *_fbt) {
    fbt_t   *fbt = fb_get_handle(_fbt);
    float    usage;
    int32_t  node_count = fbt->node_count.get();

    if (node_count) {
        usage = (fbt->ikey_count.get() + fbt->key_count.get())*100.0/
            (node_count*FB_SLOT_MAX);
    } else {
        usage = 0.0;
    }

    printf("Tree:0x%08x\n",
           reinterpret_cast<intptr_t>(fbt));
    printf("level         : %d\n", fbt->level);
    printf("ikey_count    : %lld\n", fbt->ikey_count.get());
    printf("key_count     : %lld\n", fbt->key_count.get());
    printf("node_count    : %lld\n", fbt->node_count.get());
    printf("space_usage   : %6.2f%%\n", usage);
}

/********************* Internal Functions *************/
inline fbt_t *fb_get_handle(void *_fbt) {
    return reinterpret_cast<fbt_t*>(_fbt);
}

inline fbn_t *fb_create_node() {
    return fb_node_pool.alloc();
}
inline void     fb_free_node(fbn_t *fbn) {
    fb_node_pool.free_mem(fbn);
}

inline void   fb_reset_slot(fbn_t *fbn, int32_t idx) {
    fbn->key[idx]  = FB_NIL_KEY;
//    fbn->child[idx] = fb_nil_node;
}
inline void   fb_reset_node(fbn_t *fbn, int32_t begin_idx) {
    for (int32_t i = begin_idx; i < FB_SLOT_MAX; ++i) {
        fb_reset_slot(fbn, i);
    }
}

inline void   fb_add_stat(fbt_t *fbt, fbs_t *fbs) {
    fbt->node_count.mutate(fbs->node_count);
    fbt->ikey_count.mutate(fbs->ikey_count);
    fbt->key_count.mutate(fbs->key_count);
}


inline bool   fb_w_lock(fbn_t **node) {
    return true;
//    return ywRcuRef::lock(reinterpret_cast<rcu_ptr>(node));
}
inline void   fb_w_unlock(fbn_t **node) {
    return;
//    ywRcuRef::release(reinterpret_cast<rcu_ptr>(node));
}
inline bool   fb_r_lock(fbn_t **node) {
    return true;
//    return ywRcuRef::fix(reinterpret_cast<rcu_ptr>(node));
}
inline void   fb_r_unlock(fbn_t **node) {
    return;
//    ywRcuRef::unfix(reinterpret_cast<rcu_ptr>(node));
}



inline void fb_insert_into_node(fbn_t *fbn, fbs_t *fbs,
                                fbKey key, fbn_t *child) {
    int32_t idx = fbs->cursor[ fbs->depth ].idx+1;

    assert(fbn != fb_nil_node);
    assert(idx < FB_SLOT_MAX);

    if (fbn->key[idx] != FB_NIL_KEY) {
        memmove(fbn->key+idx+1,
                fbn->key+idx,
                (FB_SLOT_MAX-idx-1) * sizeof(key));
        memmove(fbn->child+idx+1,
                fbn->child+idx,
                (FB_SLOT_MAX-idx-1) * sizeof(child));
    }
    fbn->key[idx] = key;
    fbn->child[idx] = child;
}

inline void fb_remove_in_node(fbn_t *fbn, int32_t idx) {
    assert(fbn != fb_nil_node);
    assert((0 <= idx) && (idx < FB_SLOT_MAX));
    if (fbn->key[idx+1] == FB_NIL_KEY) {
        fb_reset_slot(fbn, idx);
    } else {
        memmove(fbn->key+idx,
                fbn->key+idx+1,
                (FB_SLOT_MAX-idx-1) * sizeof(fbn->key[0]));
        memmove(fbn->child+idx,
                fbn->child+idx+1,
                (FB_SLOT_MAX-idx-1) * sizeof(fbn->child[0]));
    }
}

bool fb_split_node(fbt_t *fbt, fbn_t *fbn, fbs_t *fbs) {
    fbn_t   * fbn_right;
    fbn_t   * fbn_parent;
    int32_t   reset_idx = 0;
    int32_t   remain_cnt = 0;

    assert(fb_is_full_node(fbn));

    if (!(fbn_right = fb_create_node())) {
        return false;
    }
    fbs->node_count++;

    if (fbs->cursor[ fbs->depth ].idx == FB_LAST_SLOT-1) {
        reset_idx = FB_9_1_SPLIT;
    } else {
        if (fbs->cursor[ fbs->depth ].idx == 0) {
            reset_idx = FB_1_9_SPLIT;
        } else {
            reset_idx = FB_5_5_SPLIT;
        }
    }
    remain_cnt = FB_SLOT_MAX - reset_idx;
    memcpy(fbn_right->key,
           fbn->key + reset_idx,
           sizeof(fbn->key[0]) * remain_cnt);
    memcpy(fbn_right->child,
           fbn->child + reset_idx,
           sizeof(fbn->child[0]) * remain_cnt);
    fb_reset_node(fbn_right, remain_cnt);

    if (fbs->depth == 0) { /* It's a root node.*/
        if (!(fbn_parent = fb_create_node())) {
            fb_free_node(fbn_right);
            return false;
        }
        fbs->node_count++;
        fb_reset_node(fbn_parent, 2);

        fbn_parent->key[0]   = FB_LEFT_MOST_KEY;
        fbn_parent->child[0] = fbn;
        fbn_parent->key[1]   = fbn_right->key[0];
        fbn_parent->child[1] = fbn_right;

        fbs->ikey_count+=2;

        fbt->root = fbn_parent;
        ++fbt->level;
    } else {
        --fbs->depth;
        fbn_parent = fbs->cursor[ fbs->depth ].node;
        ++fbs->ikey_count;
        fb_insert_into_node(fbn_parent, fbs,
                            fbn_right->key[0], fbn_right);
        if (fb_is_full_node(fbn_parent)) {
            if (!fb_split_node(fbt, fbn_parent, fbs)) {
                fb_free_node(fbn_right);
                return false;
            }
        }
    }
    fb_reset_node(fbn, reset_idx);
    return true;
}

void fb_validation_node(int height, fbn_t *fbn) {
    fbKey prev_key = 0;

    for (int32_t i = 0; i < FB_SLOT_MAX; ++i) {
        assert(prev_key <= fbn->key[i]);
        prev_key = fbn->key[i];
        if (fbn->key[i] == FB_NIL_KEY) {
            break;
        }

        if (height > 1) {
            fb_validation_node(height-1, fbn->child[i]);
        }
    }
}


void fb_dump_node(int height, fbn_t *fbn) {
    printf("%12s %12s\n"
           "%12s %12s\n",
            "key", "child_ptr",
            "------------", "------------");
    for (int32_t i = 0; i < FB_SLOT_MAX; ++i) {
        if (fbn->key[i] == FB_NIL_KEY) {
            break;
        }
        printf("%12x %12x\n",
               fbn->key[i],
               reinterpret_cast<uintptr_t>(fbn->child[i]));
    }

    if (height > 1) {
        for (int32_t i = 0; i < FB_SLOT_MAX; ++i) {
            if (fbn->key[i] == FB_NIL_KEY) {
                break;
            }
            fb_dump_node(height-1, fbn->child[i]);
        }
    }
}

inline int32_t fb_search_in_node(fbn_t *fbn, fbKey key) {
    int32_t size = FB_SLOT_MAX;
    int32_t idx = 0;
    int32_t mid;

    do {
        size >>= 1;
        mid = idx + size;
        if (key >= fbn->key[mid]) {
            idx = mid;
        }
    } while (size >= 1);

    return idx;
}

inline fbn_t   *fb_traverse(fbt_t *fbt, fbKey key, fbs_t *fbs) {
    fbc_t *cursor;

    cursor = &fbs->cursor[0];
    cursor->node = fbt->root;
    for (int32_t i = 0; i < fbt->level; ++i) {
        cursor->idx = fb_search_in_node(cursor->node, key);
        (cursor+1)->node = cursor->node->child[ cursor->idx ];
        cursor++;
    }
    fbs->depth = fbt->level-1;
    return (cursor-1)->node;
}

inline bool     fb_is_full_node(fbn_t *fbn) {
    return fbn->key[FB_SLOT_MAX-1] != FB_NIL_KEY;
}
inline bool     fb_is_empty_node(fbn_t *fbn) {
    return fbn->key[0] == FB_NIL_KEY;
}

/********************* Test Functions *************/
void fb_basic_test() {
    const int32_t TEST_SIZE = 1024*32;
    void * fbt = fb_create();
    void * ret;
    int    i;

    for (i = 0; i < FB_SLOT_MAX*2; ++i) {
        fb_insert(fbt, i*2, reinterpret_cast<void*>(i*2));
    }
    for (i = 0; i < FB_SLOT_MAX*2; ++i) {
        ASSERT_TRUE(fb_remove(fbt, i*2));
    }

    for (i = 0; i < TEST_SIZE; ++i) {
        fb_insert(fbt, i*2, reinterpret_cast<void*>(i*2));
    }
    for (i = 0; i < TEST_SIZE; ++i) {
        if ((ret=fb_find(fbt, i*2))) {
            if (!fb_remove(fbt, i*2)) {
                printf("%d\n", reinterpret_cast<uint32_t>(ret));
                ASSERT_FALSE(fb_remove(fbt, i*2));
                assert(false);
            }
        }
    }

    ASSERT_TRUE(fbt);
}

const int32_t  FB_TEST_RANGE = 10;
const int32_t  FB_TEST_COUNT = 1024*64;

typedef struct _testArg {
    void * fbt;
    int    idx;
    int    data_size;
    int    try_count;
    int    op;
} testArg;

void concRoutine(void * arg) {
    testArg  *targ = reinterpret_cast<testArg*>(arg);
    int32_t   val = 0;
    int32_t   init_val =  targ->idx * targ->data_size + 1;
    int       i;
    int       j;

    switch (targ->op) {
        case 0: /*generation*/
            for (i = 0; i < targ->data_size; i ++) {
                while (!fb_insert(targ->fbt, val+init_val, NULL)) {
                }
                val = (val + 1) % targ->data_size;
            }
            break;
        case 1: /*search*/
            for (i = 0; i < targ->try_count; i ++) {
                fb_find(targ->fbt, val+init_val);
                val = (val + 1) % targ->data_size;
            }
            break;
        case 2: /*remove*/
            for (i = 0; i < targ->data_size; i ++) {
                while (!fb_remove(targ->fbt, val+init_val)) {
                }
                val = (val + 1) % targ->data_size;
            }
            break;
        case 3: /*fusion*/
            if (targ->idx == 0) {
                for (i = 0; i < targ->try_count; ++i) {
                    for (j = 0; j < FB_TEST_RANGE; ++j) {
                        fb_insert(targ->fbt,  100001 + j*2, NULL);
                    }
                    for (j = 0; j < FB_TEST_RANGE; ++j) {
                        fb_remove(targ->fbt,  100001 + j*2);
                    }
                }
            } else {
                for (i = 0; i < targ->try_count; ++i) {
                    for (j = 0; j < FB_TEST_RANGE; ++j) {
                        int32_t data = reinterpret_cast<int32_t>(
                            fb_find(targ->fbt,  100000 + j*2));
                        assert(data == 100000 + j*2);
                    }
                }
            }
            break;
    }
}

void    *test_fbt =fb_create();
void  fb_conc_test(int32_t data, int32_t try_count, int32_t op) {
    int      i;
    int      thread_count = ywThreadPool::get_processor_count();
    testArg  targ[MAX_THREAD_COUNT];

    if (op == 3) {
        for (i = 0; i < FB_TEST_RANGE; ++i) {
            ASSERT_TRUE(fb_insert(test_fbt,
                                  100000+i*2,
                                  reinterpret_cast<void*>(100000+i*2)));
        }
    }

    ywThreadPool::get_instance()->wait_to_idle();
    for (i = 0; i < thread_count; i ++) {
        targ[i].fbt       = test_fbt;
        targ[i].idx       = i;
        targ[i].data_size = data / thread_count;
        targ[i].try_count = try_count / thread_count;
        targ[i].op        = op;

        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                concRoutine, reinterpret_cast<void*>(&targ[i])));
    }
    ywThreadPool::get_instance()->wait_to_idle();

    if (op == 3) {
        for (i = 0; i < FB_TEST_RANGE; ++i) {
            ASSERT_TRUE(fb_remove(test_fbt,
                                  100000+i*2));
        }
    }

    fb_report(test_fbt);
}

