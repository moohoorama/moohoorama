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
inline fbt_t *fb_get_handle(void *_fbt);

inline fbn_t *fb_create_node();
inline void   fb_free_node(fbn_t *fbn);

fb_result _fb_insert(fbt_t *_fbt, fbKey key, fbn_t *data);

inline void   fb_flipping_root(fbt_t *fbt);
inline fbn_t *fb_get_new_root_buffer(fbt_t *fbt);
inline void   fb_reset_node(fbn_t *fnb, int32_t begin_idx = 0);
inline void   fb_insert_into_node(fbn_t *fbn, fbs_t *fbs,
                                  fbKey key, fbn_t * child);
inline void   fb_remove_in_node(fbn_t *fbn, int32_t idx);
inline void   fb_init_stack(fbs_t *fbs);
inline void   fb_add_stat(fbt_t *fbt, fbs_t *fbs);

/*********** Concurrency functions ***********/

inline bool   fb_w_lock(fbt_t *fbt, fbn_t **node);
inline void   fb_w_unlock(fbt_t *fbt, fbn_t **node);
inline bool   fb_r_lock(fbt_t *fbt, fbn_t **node);
inline void   fb_r_unlock(fbt_t *fbt, fbn_t **node);

/**************** SMO(Structure Modify Operation) *****************/
bool fb_split_node(fbt_t *fbt, fbn_t *fbn, fbs_t *fbs);

inline int32_t  fb_search_in_node(fbn_t *fbn, fbKey key);
inline fbn_t   *fb_traverse(fbt_t *fbt, fbKey key, fbs_t *fbs);
inline bool     fb_is_full_node(fbn_t *fbn);
inline bool     fb_is_empty_node(fbn_t *fbn);

inline void     fb_dump_node(int height, fbn_t *fbn);
inline void     fb_validation_node(int height, fbn_t *fbn);

void *fb_create() {
    fbt_t * fbt = fbt_tree_pool.alloc();

    if (!fbt) {
        return fb_nil_tree;
    }

    new (fbt) fbt_t();

    fbt->root_ptr   = &fbt->root_buf[0];
    new (fbt->root_ptr) fbr_t();

    fbt->ikey_count.reset();
    fbt->key_count.reset();
    fbt->node_count.reset();

    return reinterpret_cast<void*>(fbt);
}

bool  fb_insert(void *_fbt, fbKey key, void *_data) {
    fbt_t      *fbt = fb_get_handle(_fbt);
    fbn_t      *data = reinterpret_cast<fbn_t*>(_data);
    ywRcuGuard  rcuGuard(&fbt->rcu);
    fb_result   result;

    {
        ywLockGuard  lockGuard(&fbt->smo, false/*WLock*/);
        result = _fb_insert(fbt, key, data);
        if (result == FB_RESULT_FAIL) return false;
        if (result == FB_RESULT_SUCCESS) return true;
    }
    {
        ywLockGuard  lockGuard(&fbt->smo, true/*WLock*/);
        result = _fb_insert(fbt, key, data);
        if (result == FB_RESULT_SUCCESS) return true;
    }
    return false;
}
fb_result  _fb_insert(fbt_t *fbt, fbKey key, fbn_t *data) {
    fbn_t          *fbn;
    fbn_t          *new_fbn;
    fbs_t           fbs;
    ywMemPoolGuard  nodePoolGuard(&fb_node_pool);
    ywRcuGuard      rcuGuard(&fbt->rcu);

    if (!fbt->root_ptr->level) {
        if(!fbt->smo.isWLock()){
            return FB_RESULT_RETRY;
        }

        if (!(fbn = fb_create_node())) {
            fb_w_unlock(fbt, &fbt->root);
            return false;
        }
        fb_reset_node(fbn, 1);
        fbn->key[0]   = key;
        fbn->child[0] = data;

        fbr_t *fbr = fb_get_new_root_buffer(fbt);
        fbr->level = 1;
        fbr->root  = fbn;

        fb_flipping_root(fbt);

        ++fbs.node_count;
    }
    if (fbt->level) {
        fbn = fb_traverse(fbt, key, &fbs);

        if ((fbs.cursor->idx != -1) &&
            (fbn->key[ fbs.cursor->idx ] == key)) { /* dupkey */
            fb_w_unlock(fbt, &fbt->root);
            return false;
        }
        new_fbn = fb_insert_into_node(fbn, &fbs, fbs.cursor->idx+1, key, data);

        if (fb_is_full_node(fbn)) {
            if (!fb_split_node(fbt, fbn, &fbs)) {
                fb_w_unlock(fbt, &fbt->root);
                return false;
            }
        }

    } else {
        if (!fbt->smo.isWLock()) {
            if (!fbt->smo.WLock()) return false;
            bool ret = fb_insert(_fbt, key, _data); /* pessimistic */
            fbt->smo.release();
            return ret;
        }


    }

    fbs.key_count = +1;
    fb_add_stat(fbt, &fbs);

    fb_w_unlock(fbt, &fbt->root);

    return true;
}

bool  fb_remove(void *_fbt, fbKey key) {
    fbt_t *fbt = fb_get_handle(_fbt);
    fbn_t *fbn;
    fbs_t  fbs;
    fbp_t *position;

    if (!fb_w_lock(fbt, &fbt->root)) {
        return false;
    }

    if (fbt->root == fb_nil_node) {
        fb_w_unlock(fbt, &fbt->root);
        return false;
    }

    fbn = fb_traverse(fbt, key, &fbs);
    assert(fbn != fb_nil_node);

    position = &fbs.position[ fbs.depth ];
    assert(fbn == position->node);

    if (position->node->key[ position->idx] != key) {
        fb_w_unlock(fbt, &fbt->root);
        return false;
    }

    fb_remove_in_node(position->node, position->idx);

    while (fb_is_empty_node(position->node)) {
        --fbs.node_count;
        fb_free_node(position->node);
        if (fbs.depth == 0) { /* remove root*/
            fbt->root  = fb_nil_node;
            fbt->level = 0;
            break;
        }
        --fbs.depth;
        position = &fbs.position[ fbs.depth ];
        --fbs.ikey_count;
        fb_remove_in_node(position->node, position->idx);
        if (position->node->key[0] != FB_NIL_KEY) {
            position->node->key[0] = FB_LEFT_MOST_KEY;
        }
    }

    fbs.key_count = -1;
    fb_add_stat(fbt, &fbs);
    fb_w_unlock(fbt, &fbt->root);
    return true;
}

void *fb_find(void *_fbt, fbKey key) {
    fbt_t   *fbt = fb_get_handle(_fbt);
    fbn_t   *fbn = fbt->root;
    int32_t  idx;
    int32_t  i = fbt->level;

    while (!fb_r_lock(fbt, &fbt->root)) {
    }

    while (i--) {
        idx = fb_search_in_node(fbn, key);
        fbn = fbn->child[idx];
    }

    fb_r_unlock(fbt, &fbt->root);

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

inline bool   fb_w_lock(fbt_t *fbt, fbn_t **node) {
    return fbt->rcu.lock();
}
inline void   fb_w_unlock(fbt_t *fbt, fbn_t **node) {
    fbt->rcu.release();
}
inline bool   fb_r_lock(fbt_t *fbt, fbn_t **node) {
    fbt->rcu.fix();
    return true;
}
inline void   fb_r_unlock(fbt_t *fbt, fbn_t **node) {
    fbt->rcu.unfix();
}


inline void fb_flipping_root(fbt_t *fbt) {
    assert(fbt->smo.isWLock());

    fbt->root_ptr = fb_get_new_root_buffer(fbt);
}

inline fbn_t *fb_get_new_root_buffer(fbt_t *fbt) {
    assert(fbt->smo.isWLock());

    if (fbt->root_ptr == fbt->root_buf[0]) {
        return fbt->root_buf[1];
    } else {
        return fbt->root_buf[0];
    }
}

inline fbn_t *fb_insert_into_node(fbn_t *fbn, fbs_t *fbs, int32_t idx,
                                fbKey key, fbn_t *child) {
    int32_t  idx = fbs->position[ fbs->depth ].idx+1;
    int32_t  move;
    fbn_t   *new_fbn = fb_create_node();
    fbp_t   *fbc;

    if (new_fbn) {
        assert(fbn != fb_nil_node);
        assert(idx < FB_SLOT_MAX);

        if (idx) {
            memcpy(new_fbn->key,   fbn->key,   idx * sizeof(key));
            memcpy(new_fbn->child, fbn->child, idx * sizeof(child));
        }
        fbn->key[idx] = key;
        fbn->child[idx] = child;
        if (FB_LAST_SLOT > idx) {
            move = FB_LAST_SLOT - idx;
            memcpy(new_fbn->key+idx+1,   fbn->key+idx,   move*sizeof(key));
            memcpy(new_fbn->child+idx+1, fbn->child+idx, move*sizeof(child));
        }
    }
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

    if (fbs->position[ fbs->depth ].idx == FB_LAST_SLOT-1) {
        reset_idx = FB_9_1_SPLIT;
    } else {
        if (fbs->position[ fbs->depth ].idx == 0) {
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
        fbn_parent = fbs->position[ fbs->depth ].node;
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
    fbp_t *position;

    position = &fbs->position[0];
    position->node = fbt->root;
    for (int32_t i = 0; i < fbt->level; ++i) {
        position->idx = fb_search_in_node(position->node, key);
        (position+1)->node = position->node->child[ position->idx ];
        position++;
    }
    fbs->depth = fbt->level-1;
    return (position-1)->node;
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

