/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywfbtree.h>
#include <ywthread.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>
#include <ywrcuref.h>
#include <ywspinlock.h>
#include <ywmempool.h>
#include <ywfbstruct.h>

/*********** Declare Functions ***********/

fb_result _fb_insert(fbt_t *_fbt, fbKey key, fbn_t *data, bool smoLock);

/********************* Internal Functions *************/
inline fbt_t *fb_get_handle(void *_fbt);

inline fbn_t *fb_create_node(fbs_t *fbs);
inline bool   fb_free_node(fbs_t *fbs, fbn_t *fbn);
inline void   fb_reset_slot(fbn_t *fbn, int32_t idx);
inline void   fb_reset_node(fbn_t *fnb, int32_t begin_idx = 0);

/********************* Cloning *************/
inline bool   fb_set_newversion(fbs_t *fbs, fbn_t *old_n, fbn_t *new_n);
inline void   fb_copy_node(fbn_t *src, int32_t begin, int32_t count,
                           fbn_t *dst, int32_t target);
inline fbn_t *fb_cloning_and_insert(fbn_t *fbn, fbs_t *fbs,
                                    fbKey key, fbn_t * child);
inline void   fb_remove_in_node(fbn_t *fbn, int32_t idx);

/**************** SMO(Structure Modify Operation) *****************/
inline fbn_t *fb_make_new_root(fbs_t *fbs,
                               fbKey key1, fbn_t * child1,
                               fbKey key2, fbn_t * child2);
inline void   fb_change_root(fbt_t *fbt, int32_t level, fbn_t *new_root);
inline fbr_t *fb_get_new_root_buffer(fbt_t *fbt);

bool          fb_split_node(fbs_t *fbs);

inline bool   fb_propogate(fbt_t *fbt, fbs_t *fbs, fbn_t *old_n, fbn_t *new_n);

inline int32_t  fb_search_in_node(fbn_t *fbn, fbKey key);
inline fbn_t   *fb_traverse(fbs_t *fbs, fbKey key);
inline bool     fb_is_full_node(fbn_t *fbn);
inline bool     fb_is_empty_node(fbn_t *fbn);

inline void     fb_dump_node(int height, fbn_t *fbn);
inline void     fb_validation_node(
    int height, fbn_t *fbn, fbKey begin_key, fbKey end_key);

static ywMemPool<fbn_t>             fb_node_pool;
static ywMemPool<fbt_t>             fbt_tree_pool;

static fbn_t  fb_nil_node_instance;
static fbn_t *fb_nil_node=&fb_nil_node_instance;

static fbt_t  fb_nil_tree_instance;
static fbt_t *fb_nil_tree=&fb_nil_tree_instance;

fbNodeStruct::fbNodeStruct() {
}

fbRootStruct::fbRootStruct()
    :root(fb_nil_node), level(0) {
}

fbTreeStruct::fbTreeStruct() {
    if (this == &fb_nil_tree_instance) {
        root_ptr   = &root_buf[0];
        new (root_ptr) fbr_t();
    }
}
fbStackStruct::fbStackStruct(fbt_t *_fbt):
    fbt(_fbt), cursor(NULL), depth(0), org_root(fbt->root_ptr),
    key_count(0), node_count(0), reuse_node_count(0),
    free_node_idx(0),
    nodePoolGuard(&fb_node_pool) {
}


void *fb_create() {
    fbt_t * fbt = fbt_tree_pool.alloc();

    if (!fbt) {
        return fb_nil_tree;
    }

    new (fbt) fbt_t();

    fbt->root_ptr   = &fbt->root_buf[0];
    new (fbt->root_ptr) fbr_t();

    fbt->key_count.reset();
    fbt->node_count.reset();
    fbt->free_node_count.reset();

    return reinterpret_cast<void*>(fbt);
}

bool  fb_insert(void *_fbt, fbKey key, void *_data) {
    fbt_t      *fbt = fb_get_handle(_fbt);
    fbn_t      *data = reinterpret_cast<fbn_t*>(_data);
    ywRcuGuard  rcuGuard(&fbt->rcu);
    bool        smoLock = false;

    while (true) {
        switch (_fb_insert(fbt, key, data, smoLock)) {
            case FB_RESULT_SUCCESS:
                return true;
            case FB_RESULT_FAIL:
            case FB_RESULT_DUP:
            case FB_RESULT_LOW_RESOURCE:
                return false;
            case FB_RESULT_SMO:
                assert(smoLock == false);
                smoLock = true;
                break;
            case FB_RESULT_RETRY:
                break;
        }
    }
}
fb_result  _fb_insert(fbt_t *fbt, fbKey key, fbn_t *data, bool smoLock) {
    fbn_t          *fbn;
    fbn_t          *new_fbn;
    fbs_t           fbs(fbt);
    ywLockGuard<1>  smoGuard;

    if ((fbt->root_ptr->level <= 1) && (!smoLock)) {
        return FB_RESULT_SMO;    /* No Root, or Root only*/
    }

    if (smoLock) {
        if (!smoGuard.WLock(&fbt->smo)) {
            return FB_RESULT_LOW_RESOURCE;
        }
    } else {
        if (!smoGuard.RLock(&fbt->smo)) {
            return FB_RESULT_LOW_RESOURCE;
        }
    }

    if (!fbt->root_ptr->level) {
        if (!smoLock) return FB_RESULT_SMO;
        if (!fb_make_new_root(&fbs,
                              key, data,
                              FB_NIL_KEY, fb_nil_node)) {
            return FB_RESULT_LOW_RESOURCE;
        }
    } else {
        fbn = fb_traverse(&fbs, key);

        if ((fbs.cursor->idx != -1) &&
            (fbn->key[ fbs.cursor->idx ] == key)) { /* dupkey */
            return FB_RESULT_DUP;;
        }

        /* lock to target leaf*/
        if (!(fbs.node_lock.WLock(&fbn->lock))) return FB_RESULT_LOW_RESOURCE;

        if (fb_is_full_node(fbn)) { /* need smo*/
            if (!smoLock) return FB_RESULT_SMO;
            if (!fb_split_node(&fbs)) return FB_RESULT_LOW_RESOURCE;
            fbs.commit();
            return FB_RESULT_RETRY;
        }

        new_fbn = fb_cloning_and_insert(fbn, &fbs, key, data);
        if (!new_fbn) {
            return FB_RESULT_LOW_RESOURCE;
        }

        fbs.stack_up();
        if (!fb_set_newversion(&fbs, fbn, new_fbn)) {
            return FB_RESULT_LOW_RESOURCE;
        }
    }
    fbs.key_count = +1;
    fbs.commit();

    return FB_RESULT_SUCCESS;
}

bool  fb_remove(void *_fbt, fbKey key) {
    fbt_t          *fbt = fb_get_handle(_fbt);
    fbs_t           fbs(fbt);
    ywRcuGuard      rcuGuard(&fbt->rcu);
    fbn_t          *fbn;
    ywLockGuard<1>  smoGuard;

    if (fbt->root_ptr->level == 0) {
        return false;
    }
    fbn = fb_traverse(&fbs, key);
    if ((fbn == fb_nil_node) ||
        (fbn->key[ fbs.cursor->idx ] != key)) {
        return false;
    }

    /* lock to target leaf*/
    if (!(fbs.node_lock.WLock(&fbn->lock))) return FB_RESULT_LOW_RESOURCE;
    fb_remove_in_node(fbn, fbs.cursor->idx);

    if (fb_is_empty_node(fbs.cursor->node)) {
        if (!smoGuard.WLock(&fbt->smo)) {
            return FB_RESULT_LOW_RESOURCE;
        }

        while (fb_is_empty_node(fbs.cursor->node)) {
            if (!fb_free_node(&fbs, fbs.cursor->node)) return false;
            if (fbs.depth == 0) {
                fb_change_root(fbt, 0, fb_nil_node);
                break;
            }
            fbs.stack_up();
            fb_remove_in_node(fbs.cursor->node, fbs.cursor->idx);
            if (fbs.cursor->node->key[0] != FB_NIL_KEY) {
                fbs.cursor->node->key[0] = FB_LEFT_MOST_KEY;
            }
        }
    }

    fbs.key_count = -1;
    fbs.commit();

    return true;
}

void *fb_find(void *_fbt, fbKey key) {
    fbt_t      *fbt = fb_get_handle(_fbt);
    fbr_t      *fbr = fbt->root_ptr;
    fbn_t      *fbn = fbr->root;
    int32_t     idx;
    int32_t     i = fbr->level;
    ywRcuGuard  rcuGuard(&fbt->rcu);

    while (i--) {
        idx = fb_search_in_node(fbn, key);
        fbn = fbn->child[idx];
    }

    return fbn;
}

void  fb_dump(void *_fbt) {
    fbt_t      *fbt = fb_get_handle(_fbt);
    fbr_t      *fbr = fbt->root_ptr;
    ywRcuGuard  rcuGuard(&fbt->rcu);

    printf("==================================\n");
    fb_dump_node(fbr->level, fbr->root);
}

void fb_validation(void *_fbt) {
    fbt_t      *fbt = fb_get_handle(_fbt);
    fbr_t      *fbr = fbt->root_ptr;
    ywRcuGuard  rcuGuard(&fbt->rcu);

    fb_validation_node(fbr->level, fbr->root, 0, FB_NIL_KEY);
}

void fb_report(void *_fbt) {
    fbt_t      *fbt = fb_get_handle(_fbt);
    float       usage;
    int64_t     node_count = fbt->node_count.get();
    int64_t     ikey_count = node_count - 1;
    ywRcuGuard  rcuGuard(&fbt->rcu);

    if (node_count) {
        usage = (ikey_count + fbt->key_count.get())*100.0/
            (node_count*FB_SLOT_MAX);
    } else {
        usage = 0.0;
    }

    printf("Tree:0x%08" PRIxPTR "\n",
           reinterpret_cast<intptr_t>(fbt));
    printf("level            : %" PRId32 "\n", fbt->root_ptr->level);
    printf("ikey_count       : %" PRId64 "\n", ikey_count);
    printf("key_count        : %" PRId64 "\n", fbt->key_count.get());
    printf("free_node_count  : %" PRId64 "\n", fbt->free_node_count.get());
    printf("node_count       : %" PRId64 "\n", node_count);
    printf("space_usage      : %6.2f%%\n", usage);
}

/********************* Internal Functions *************/
inline fbt_t *fb_get_handle(void *_fbt) {
    return reinterpret_cast<fbt_t*>(_fbt);
}

inline fbn_t *fb_create_node(fbs_t *fbs) {
    fbn_t * ret = reinterpret_cast<fbn_t*>(fbs->fbt->rcu.get_reusable_item());
    if (ret) {
        ++fbs->reuse_node_count;
        fbs->nodePoolGuard.regist(ret);
    } else {
        ret = fbs->nodePoolGuard.alloc();
    }
    if (ret) {
        ++fbs->node_count;
        new (ret) fbNodeStruct();
    }
    return ret;
}
inline bool     fb_free_node(fbs_t *fbs, fbn_t *fbn) {
    if (!(fbs->push_free_node(fbn))) return false;

    return true;
}

inline void   fb_reset_slot(fbn_t *fbn, int32_t idx) {
    fbn->key[idx]  = FB_NIL_KEY;
}
inline void   fb_reset_node(fbn_t *fbn, int32_t begin_idx) {
    for (int32_t i = begin_idx; i < FB_SLOT_MAX; ++i) {
        fb_reset_slot(fbn, i);
    }
}

/*********************** Cloning ***********************/
inline bool  fb_set_newversion(fbs_t *fbs, fbn_t *old_n, fbn_t *new_n) {
    fbn_t * parent;
    if (fbs->depth >= 0) {
        parent = fbs->cursor->node;
        if (!(fbs->node_lock.WLock(&parent->lock))) return false;
        assert(parent->child[ fbs->cursor->idx ] == old_n);
        parent->child[ fbs->cursor->idx ] = new_n;
        if (!(fbs->push_free_node(old_n))) return false;
    } else {
        fb_change_root(fbs->fbt, fbs->fbt->root_ptr->level, new_n);
    }
    return true;
}

inline void   fb_copy_node(fbn_t *src, int32_t begin, int32_t cnt,
                           fbn_t *dst, int32_t target) {
    memcpy(dst->key+target,   src->key+begin,   cnt * sizeof(src->key[0]));
    memcpy(dst->child+target, src->child+begin, cnt * sizeof(src->child[0]));
}

inline fbn_t *fb_cloning_and_insert(fbn_t *fbn, fbs_t *fbs,
                                    fbKey key, fbn_t * child) {
    int32_t  idx = fbs->cursor->idx + 1;
    fbn_t   *new_fbn = fb_create_node(fbs);

    if (!new_fbn) return NULL;

    assert(fbn != fb_nil_node);
    assert(idx < FB_SLOT_MAX);

    if (idx) {
        fb_copy_node(fbn, 0, idx, new_fbn, 0);
    }
    if (FB_LAST_SLOT > idx) {
        fb_copy_node(fbn, idx, FB_LAST_SLOT-idx, new_fbn, idx+1);
    }

    new_fbn->key[idx] = key;
    new_fbn->child[idx] = child;

    return new_fbn;
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

/*********************** SMO ***********************/
inline fbn_t *fb_make_new_root(fbs_t *fbs,
                               fbKey key1, fbn_t * child1,
                               fbKey key2, fbn_t * child2) {
    fbn_t *fbn = fb_create_node(fbs);

    if (!fbn)  return NULL;

    fb_reset_node(fbn, 2);
    fbn->key[0]   = key1;
    fbn->child[0] = child1;
    fbn->key[1]   = key2;
    fbn->child[1] = child2;

    fb_change_root(fbs->fbt, fbs->fbt->root_ptr->level + 1, fbn);

    return fbn;
}
inline void fb_change_root(fbt_t *fbt, int32_t level, fbn_t * new_root) {
    fbr_t *fbr = fb_get_new_root_buffer(fbt);

    assert(fbr != fbt->root_ptr);
    assert(fbt->smo.hasWLock());

    fbr->level = level;
    fbr->root  = new_root;

    fbt->root_ptr = fbr;
}

inline fbr_t *fb_get_new_root_buffer(fbt_t *fbt) {
    assert(fbt->smo.hasWLock());

    if (fbt->root_ptr == &fbt->root_buf[0]) {
        return &fbt->root_buf[1];
    } else {
        return &fbt->root_buf[0];
    }
}

bool fb_split_node(fbs_t *fbs) {
    fbn_t   * fbn = fbs->cursor->node;
    fbn_t   * fbn_left;
    fbn_t   * fbn_right;
    fbn_t   * fbn_parent;
    fbn_t   * fbn_new_parent;
    int32_t   split_idx = 0;
    int32_t   remain_cnt = 0;
    bool      move_to_right = false;

    assert(fb_is_full_node(fbn));

    if (!(fbn_left = fb_create_node(fbs))) return false;
    if (!(fbn_right = fb_create_node(fbs))) return false;

    if (fbs->cursor->idx == FB_LAST_SLOT) {
        split_idx = FB_LAST_SLOT;
        move_to_right = true;
    } else {
        if (fbs->cursor->idx == 0) {
            split_idx = 1;
        } else {
            split_idx = FB_5_5_SPLIT;
            if (fbs->cursor->idx >= split_idx) {
                move_to_right = true;
            }
        }
    }
    remain_cnt = FB_SLOT_MAX - split_idx;

    fb_copy_node(fbn, 0, split_idx,          fbn_left, 0);
    fb_reset_node(fbn_left, split_idx);

    fb_copy_node(fbn, split_idx, remain_cnt, fbn_right, 0);
    fb_reset_node(fbn_right, remain_cnt);

    if (move_to_right) {
        fbs->cursor->node = fbn_right;
        fbs->cursor->idx  = fbs->cursor->idx - split_idx;
    } else {
        fbs->cursor->node = fbn_left;
    }
    assert(fbs->cursor->node->key[fbs->cursor->idx] != FB_NIL_KEY);

    if (fbs->depth == 0) { /* It's a root node.*/
        if (!(fbn_parent = fb_make_new_root(fbs, FB_LEFT_MOST_KEY, fbn_left,
                                            fbn_right->key[0], fbn_right) )) {
            return false;
        }

        /* Since a process make a new root node, 
         * A process must add new root position to the stack.*/
        if (move_to_right) {
            fbs->add_new_root_stack(fbn_parent, 1);
        } else {
            fbs->add_new_root_stack(fbn_parent, 0);
        }
    } else {
        fbs->stack_up();
        fbn_parent = fbs->cursor->node;
        if (fb_is_full_node(fbn_parent)) {
            if (!fb_split_node(fbs)) return false;
            fbn_parent = fbs->cursor->node; /* get New Praent*/
        }

        assert(fbn_parent->child[fbs->cursor->idx] == fbn);

        fbn_new_parent = fb_cloning_and_insert(fbn_parent, fbs,
                                               fbn_right->key[0], fbn_right);
        if (!fbn_new_parent) return false;
        fbs->cursor->node = fbn_new_parent;

        if (!fb_set_newversion(fbs, fbn, fbn_left)) return false;
        if (move_to_right) {
            fbs->cursor->idx++;
        }

        fbs->stack_up();
        if (!fb_set_newversion(fbs, fbn_parent, fbn_new_parent)) {
            return false;
        }
        fbs->stack_down();
    }

    fbs->stack_down();
    return true;
}

void fb_validation_node(int height, fbn_t *fbn,
                        fbKey begin_key, fbKey end_key) {
    fbKey prev_key = 0;
    fbKey _begin_key;
    fbKey _end_key;

    for (int32_t i = 0; i < FB_SLOT_MAX; ++i) {
        assert(prev_key <= fbn->key[i]);
        prev_key = fbn->key[i];
        if (fbn->key[i] == FB_NIL_KEY) {
            break;
        }

        if (height > 1) { /*internal node */
            _begin_key = fbn->key[i];
            if (i == FB_LAST_SLOT) {
                _end_key = FB_NIL_KEY;
            } else {
                _end_key = fbn->key[i+1];
            }
            fb_validation_node(height-1, fbn->child[i],
                               _begin_key, _end_key);

            if (i > 0) {
                assert((begin_key <= fbn->key[i]) && (fbn->key[i] <= end_key));
            }
        } else {
            assert((begin_key <= fbn->key[i]) && (fbn->key[i] <= end_key));
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
        printf("%12" PRId32 " %12" PRIxPTR "\n",
               fbn->key[i],
               reinterpret_cast<intptr_t>(fbn->child[i]));
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

inline fbn_t   *fb_traverse(fbs_t *fbs, fbKey key) {
    fbt_t *fbt = fbs->fbt;
    fbr_t *fbr = fbt->root_ptr;

    fbs->position[0].node = fbr->root;
    for (int32_t i = 0; i < fbr->level; ++i) {
        fbs->cursor = &fbs->position[i];
        fbs->cursor->idx = fb_search_in_node(fbs->cursor->node, key);
        (fbs->cursor+1)->node = fbs->cursor->node->child[ fbs->cursor->idx ];
    }
    fbs->depth = fbr->level-1;
    return fbs->cursor->node;
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

    for (i = 0; i < FB_SLOT_MAX*2*1024; ++i) {
        fb_insert(fbt, i*2, reinterpret_cast<void*>(i*2));
//        fb_validation(fbt);
//        fb_dump(fbt);
    }
    for (i = 0; i < FB_SLOT_MAX*2; ++i) {
        ASSERT_TRUE(fb_remove(fbt, i*2));
        fb_validation(fbt);
    }

    for (i = 0; i < TEST_SIZE; ++i) {
        fb_insert(fbt, i*2, reinterpret_cast<void*>(i*2));
    }
    for (i = 0; i < TEST_SIZE; ++i) {
        if ((ret=fb_find(fbt, i*2))) {
            if (!fb_remove(fbt, i*2)) {
                printf("%" PRIdPTR "\n", reinterpret_cast<intptr_t>(ret));
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
                        intptr_t data = reinterpret_cast<intptr_t>(
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

