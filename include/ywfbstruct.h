/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
/* FixedsizeBTree(FBTree)*/

#ifndef INCLUDE_YWFBSTRUCT_H_
#define INCLUDE_YWFBSTRUCT_H_

#include <ywcommon.h>

static const int32_t FB_SLOT_MAX       = 128;
static const int32_t FB_LAST_SLOT      = FB_SLOT_MAX-1;
static const int32_t FB_5_5_SPLIT      = FB_SLOT_MAX/2;
static const fbKey   FB_NIL_KEY        = UINT32_MAX;
static const fbKey   FB_LEFT_MOST_KEY  = 0;
static const int32_t FB_DEPTH_MAX      = 10;

static const int32_t FB_NODELOCK_MAX   = 16;

typedef struct       fbNodeStruct     fbn_t;
typedef struct       fbRootStruct     fbr_t;
typedef struct       fbTreeStruct     fbt_t;
typedef struct       fbStackStruct    fbs_t;
typedef struct       fbPositionStruct fbp_t;

typedef enum {
    FB_RESULT_SUCCESS,
    FB_RESULT_FAIL,
    FB_RESULT_DUP,
    FB_RESULT_RETRY,
    FB_RESULT_LOW_RESOURCE,
    FB_RESULT_SMO
} fb_result;


/************* DataStructure **********/
struct fbNodeStruct {
    explicit fbNodeStruct();

    fbKey       key[FB_SLOT_MAX];
    fbn_t      *child[FB_SLOT_MAX];
    ywSpinLock  lock;
};

struct fbRootStruct {
    fbRootStruct();
    fbn_t          *root;
    int32_t         level;
};

struct fbTreeStruct {
    fbTreeStruct();

    fbr_t                 *root_ptr;
    fbr_t                  root_buf[2];

    ywSpinLock             smo;
    ywRcuRef               rcu;

    ywAccumulator<int64_t> key_count;
    ywAccumulator<int64_t> node_count;
};

struct fbPositionStruct {
    fbn_t   *node;
    int32_t  idx;
};

struct fbStackStruct {
    explicit fbStackStruct(fbt_t *_fbt);

    bool WLock(ywSpinLock *lock) {
        if (node_lock_idx >= FB_NODELOCK_MAX) return false;

        node_lock[ node_lock_idx ].set(lock);
        if (node_lock[ node_lock_idx ].WLock()) {
            node_lock_idx++;
            return true;
        }
        return false;
    }
    bool RLock(ywSpinLock *lock) {
        if (node_lock_idx >= FB_NODELOCK_MAX) return false;

        node_lock[ node_lock_idx ].set(lock);
        if (node_lock[ node_lock_idx ].RLock()) {
            node_lock_idx++;
            return true;
        }
        return false;
    }
    void commit() {
        nodePoolGuard.commit();
        fbt->node_count.mutate(node_count);
        fbt->key_count.mutate(key_count);
        fbt = NULL;
    }
    void stack_up() {
        --depth;
        --cursor;
    }
    fbt_t                 *fbt;
    fbp_t                  position[FB_DEPTH_MAX];
    fbp_t                 *cursor;
    int32_t                depth;
    int32_t                key_count;
    int32_t                node_count;

    int32_t                node_lock_idx;
    ywLockGuard            node_lock[FB_NODELOCK_MAX];
    ywMemPoolGuard<fbn_t>  nodePoolGuard;
};

#endif  // INCLUDE_YWFBSTRUCT_H_

