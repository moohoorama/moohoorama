/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywfbtree.h>
#include <ywthread.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>
#include <ywmempool.h>

static const int32_t FB_SLOT_MAX       = 128;
static const int32_t FB_LAST_SLOT      = FB_SLOT_MAX-1;
static const int32_t FB_SPLITPOINT     = FB_SLOT_MAX/2;
static const fbKey   FB_NIL_KEY        = UINT32_MAX;
static const fbKey   FB_LEFT_MOST_KEY  = 0;
static const int32_t FB_DEPTH_MAX      = 10;

typedef struct       fbNodeStruct   fbn_t;
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

struct fbTreeStruct {
    fbn_t *root;
    int    level;
    int    count;
};

struct fbCursorStruct {
    fbn_t   *node;
    int32_t  idx;
};

struct fbStackStruct {
    fbc_t   cursor[FB_DEPTH_MAX];
    int32_t depth;
};

fbn_t  fb_nil_node_instance;
fbn_t *fb_nil_node=&fb_nil_node_instance;
fbt_t  fb_nil_tree_instance = {fb_nil_node, 0};
fbt_t *fb_nil_tree=&fb_nil_tree_instance;

ywMemPool<fbn_t>             fb_node_pool;
ywMemPool<fbt_t>             fbt_tree_pool;

void *fb_create() {
    fbt_t * fbt = fbt_tree_pool.alloc();

    if (!fbt) {
        return fb_nil_tree;
    }

    fbt->root  = fb_nil_node;
    fbt->level = 0;
    fbt->count = 0;

    return reinterpret_cast<void*>(fbt);
}

bool  fb_insert(void *_fbt, fbKey key, void *_data) {
    fbt_t *fbt = fb_get_handle(_fbt);
    fbn_t *data = reinterpret_cast<fbn_t*>(_data);
    fbn_t *fbn;
    fbs_t  fbs;

    if (fbt->level) {
        fbn = fb_traverse(fbt, key, &fbs);
    } else {
        if (!(fbn = fb_create_node())) {
            return false;
        }
        fb_reset_node(fbn);
        fbt->root = fbn;
        ++fbt->level;

        fbs.depth = 0;
        fbs.cursor[0].node = fbn;
        fbs.cursor[0].idx  = -1;
    }

    if ((fbs.cursor[ fbs.depth ].idx != -1) &&
        (fbn->key[ fbs.cursor[ fbs.depth ].idx ] == key)) { /* dupkey */
        return false;
    }
    fb_insert_into_node(fbn, &fbs, key, data);
    ++fbt->count;

    if (fb_is_full_node(fbn)) {
        return fb_split_node(fbt, fbn, &fbs);
    }

    return true;
}

bool  fb_remove(void *_fbt, fbKey key) {
    fbt_t *fbt = fb_get_handle(_fbt);
    fbn_t *fbn;
    fbs_t  fbs;
    fbc_t *cursor;

    if (fbt->root == fb_nil_node) {
        return false;
    }

    fbn = fb_traverse(fbt, key, &fbs);
    assert(fbn != fb_nil_node);

    cursor = &fbs.cursor[ fbs.depth ];
    assert(fbn == cursor->node);

    if (cursor->node->key[ cursor->idx] != key) {
        return false;
    }

    fb_remove_in_node(cursor->node, cursor->idx);

    while (fb_is_empty_node(cursor->node)) {
        fb_free_node(cursor->node);
        if (fbs.depth == 0) { /* remove root*/
            fbt->root  = fb_nil_node;
            fbt->level = 0;
            break;
        }
        --fbs.depth;
        cursor = &fbs.cursor[ fbs.depth ];
        fb_remove_in_node(cursor->node, cursor->idx);
        if (cursor->node->key[0] != FB_NIL_KEY) {
            cursor->node->key[0] = FB_LEFT_MOST_KEY;
        }
    }

    --fbt->count;
    return true;
}

void *fb_find(void *_fbt, fbKey key) {
    fbt_t   *fbt = fb_get_handle(_fbt);
    fbn_t   *fbn = fbt->root;
    int32_t  idx;
    for (int32_t i = 0; i < fbt->level; ++i) {
        idx = fb_search_in_node(fbn, key);
        fbn = fbn->child[idx];
    }

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
    fbn->child[idx] = fb_nil_node;
}
inline void   fb_reset_node(fbn_t *fbn, int32_t begin_idx) {
    for (int32_t i = begin_idx; i < FB_SLOT_MAX; ++i) {
        fb_reset_slot(fbn, i);
    }
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
    fb_reset_node(fbn_right);

    if (fbs->cursor[ fbs->depth ].idx == FB_LAST_SLOT-1) {
        /* 9:1 split */
        reset_idx = FB_LAST_SLOT;
    } else {
        if (fbs->cursor[ fbs->depth ].idx == 0) {
            /* 1:9 split */
            reset_idx = 1;
        } else {
            /* 5:5 split*/
            reset_idx = FB_SPLITPOINT;
        }
    }
    remain_cnt = FB_SLOT_MAX - reset_idx;
    memcpy(fbn_right->key,
           fbn->key + reset_idx,
           sizeof(fbn->key[0]) * remain_cnt);
    memcpy(fbn_right->child,
           fbn->child + reset_idx,
           sizeof(fbn->child[0]) * remain_cnt);

    if (fbs->depth == 0) { /* It's a root node.*/
        if (!(fbn_parent = fb_create_node())) {
            fb_free_node(fbn_right);
            return false;
        }
        fb_reset_node(fbn_parent);

        fbn_parent->key[0]   = FB_LEFT_MOST_KEY;
        fbn_parent->child[0] = fbn;
        fbn_parent->key[1]   = fbn_right->key[0];
        fbn_parent->child[1] = fbn_right;

        fbt->root = fbn_parent;
        ++fbt->level;
    } else {
        --fbs->depth;
        fbn_parent = fbs->cursor[ fbs->depth ].node;
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

    do {
        size >>= 1;
        idx += size*(key >= fbn->key[idx + size]);
    } while (size > 1);

    idx -= (key < fbn->key[idx]);
    if (idx < 0) {
        idx = 0;
    }
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
    /*
    for (i = 0; i < TEST_SIZE*2; ++i) {
        ASSERT_EQ(i & ~1, reinterpret_cast<uint32_t>(fb_find(fbt, i)));
    }
    */
    for (i = 0; i < TEST_SIZE; ++i) {
        if ((ret=fb_find(fbt, i*2))) {
            if (!fb_remove(fbt, i*2)) {
                printf("%d\n", reinterpret_cast<uint32_t>(ret));
                ASSERT_FALSE(fb_remove(fbt, i*2));
                assert(false);
            }
        }
    }
    /*
    for (int32_t j = 0; j < 1024*8; ++j) {
        for (i = 0; i < 1024; ++i) {
            ASSERT_EQ(fb_find(fbt, i+100), reinterpret_cast<void*>(i+100));
        }
    }
    */

    ASSERT_TRUE(fbt);
}


