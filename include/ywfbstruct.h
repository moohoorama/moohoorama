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

static const int32_t FB_STACK_SIZE     = 16;

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
    ywAccumulator<int64_t> free_node_count;
};

struct fbPositionStruct {
    fbn_t   *node;
    int32_t  idx;
};

struct fbStackStruct {
    explicit fbStackStruct(fbt_t *_fbt);
    ~fbStackStruct() {
        if (fbt) {
            rollback();
        }
    }

    bool check_dup_lock(ywSpinLock *lock, bool wlock) {
        for (int32_t i = 0; i < node_lock_idx; ++i) {
            if (node_lock[ i ].isEqual(lock)) {
                if (wlock) {
                    return node_lock[ i ].hasWLock();
                }
                return true;
            }
        }
        return false;
    }

    bool WLock(ywSpinLock *lock) {
        if (node_lock_idx >= FB_STACK_SIZE) return false;

        if (check_dup_lock(lock, true/*WLock*/)) {
            return true;
        }

        node_lock[ node_lock_idx ].set(lock);
        if (node_lock[ node_lock_idx ].WLock()) {
            node_lock_idx++;
            return true;
        }
        return false;
    }
    bool RLock(ywSpinLock *lock) {
        if (node_lock_idx >= FB_STACK_SIZE) return false;

        if (check_dup_lock(lock, false/*WLock*/)) {
            return true;
        }

        node_lock[ node_lock_idx ].set(lock);
        if (node_lock[ node_lock_idx ].RLock()) {
            node_lock_idx++;
            return true;
        }
        return false;
    }

    bool push_free_node(fbn_t *fbn) {
        if (free_node_idx >= FB_STACK_SIZE) return false;

        free_node[free_node_idx++] = fbn;

        return true;
    }

    void commit() {
        fbt->free_node_count.mutate(free_node_idx - reuse_node_count);
        fbt->node_count.mutate(node_count - free_node_idx);
        fbt->key_count.mutate(key_count);
        nodePoolGuard.commit();
        if (free_node_idx) {
            fbt->rcu.lock();
            while (free_node_idx--) {
                fbt->rcu.regist_free_obj(free_node[free_node_idx]);
            }
            fbt->rcu.release();
        }
        fbt = NULL;
    }
    void rollback() {
        nodePoolGuard.rollback();
        fbt->root_ptr = org_root;
        free_node_idx = 0;
        fbt = NULL;
    }

    void add_new_root_stack(fbn_t *node, int32_t idx) {
        memmove(position+1, position,
                sizeof(position[0])*(fbt->root_ptr->level-1));
        position[0].node = node;
        position[0].idx  = idx;
    }
    void stack_up() {
        --depth;
        --cursor;
    }
    void stack_down() {
        ++depth;
        ++cursor;
    }

    fbt_t                 *fbt;
    fbp_t                  position[FB_DEPTH_MAX];
    fbp_t                 *cursor;
    int32_t                depth;
    fbr_t                 *org_root;

    int32_t                key_count;
    int32_t                node_count;
    int32_t                reuse_node_count;
    int32_t                node_lock_idx;
    ywLockGuard            node_lock[FB_STACK_SIZE];

    int32_t                free_node_idx;
    fbn_t                 *free_node[FB_STACK_SIZE];

    ywMemPoolGuard<fbn_t>  nodePoolGuard;
};

#endif  // INCLUDE_YWFBSTRUCT_H_

