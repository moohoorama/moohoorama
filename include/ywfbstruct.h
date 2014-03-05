/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
/* FixedsizeBTree(FBTree)*/

#ifndef INCLUDE_YWFBSTRUCT_H_
#define INCLUDE_YWFBSTRUCT_H_

#include <ywcommon.h>

static const int32_t FB_SLOT_MAX       = 128;
static const int32_t FB_LAST_SLOT      = FB_SLOT_MAX-1;
static const int32_t FB_5_5_SPLIT      = FB_SLOT_MAX/2;
static const int32_t FB_1_9_SPLIT      = 1;
static const int32_t FB_9_1_SPLIT      = FB_LAST_SLOT;
static const fbKey   FB_NIL_KEY        = UINT32_MAX;
static const fbKey   FB_LEFT_MOST_KEY  = 0;
static const int32_t FB_DEPTH_MAX      = 10;

typedef struct       fbNodeStruct    fbn_t;
typedef struct       fbRootStruct    fbr_t;
typedef struct       fbTreeStruct    fbt_t;
typedef struct       fbStackStruct   fbs_t;
typedef struct       fbPositionStruct fbp_t;

typedef enum {
    FB_RESULT_FAIL,
    FB_RESULT_SUCCESS,
    FB_RESULT_RETRY,
    FB_RESULT_SMO
} fb_result;


/************* DataStructure **********/
struct fbNodeStruct {
    explicit fbNodeStruct() {
        fb_reset_node(this);
    }
    fbKey   key[FB_SLOT_MAX];
    fbn_t  *child[FB_SLOT_MAX];
};
static fbn_t  fb_nil_node_instance;
static fbn_t *fb_nil_node=&fb_nil_node_instance;

struct fbRootStruct {
    fbRootStruct():root(fb_nil_node), level(0) {
    }
    fbn_t          *root;
    int32_t         level;
};

struct fbTreeStruct {
    fbTreeStruct();

    fbr_t                 *root_ptr;
    fbr_t                  root_buf[2];

    ywSpinLock             smo;
    ywRcuRef               rcu;

    ywAccumulator<int64_t> ikey_count;
    ywAccumulator<int64_t> key_count;
    ywAccumulator<int64_t> node_count;
};
static fbt_t  fb_nil_tree_instance;
static fbt_t *fb_nil_tree=&fb_nil_tree_instance;

fbTreeStruct::fbTreeStruct() {
    if (this == &fb_nil_tree_instance) {
        root = fb_nil_node;
    }
}

struct fbPositionStruct {
    fbn_t   *node;
    int32_t  idx;
};

struct fbStackStruct {
    fbStackStruct():
        cursor(NULL), depth(0),
        ikey_count(0), key_count(0), node_count(0),
        nodePoolGuard(&fb_node_pool) {
    }
    fbp_t           position[FB_DEPTH_MAX];
    fbp_t          *cursor;
    int32_t         depth;
    int32_t         ikey_count;
    int32_t         key_count;
    int32_t         node_count;

    ywMemPoolGuard  nodePoolGuard;
};

static ywMemPool<fbn_t>             fb_node_pool;
static ywMemPool<fbt_t>             fbt_tree_pool;


#endif  // INCLUDE_YWFBSTRUCT_H_
