/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrbtree.h>
#include <ywthread.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>
#include <ywmempool.h>

/********************* Data structure ******************/
typedef int32_t node_color_t;
static const node_color_t RB_RED     = 0;
static const node_color_t RB_BLACK   = 1;

typedef int32_t node_side_t;
static const node_side_t RB_LEFT     = 0;
static const node_side_t RB_RIGHT    = 1;
static const node_side_t RB_SIDE_MAX = 2;

typedef struct rbNodeStruct node_t;
struct rbNodeStruct {
    node_color_t       color;
    node_t           * parent;
    node_t           * child[RB_SIDE_MAX];
    key_t              key;
    void             * data;
};

typedef struct rbTreeStruct rb_t;
struct rbTreeStruct {
    node_t  * root;
    char      reserved_area[28];
};

#define LEFT_CHILD(node) ((node)->child[RB_LEFT])
#define RIGHT_CHILD(node) ((node)->child[RB_RIGHT])
#define debug(...)

node_t nil_node_instance={RB_BLACK, &nil_node_instance,
    {&nil_node_instance, &nil_node_instance}, 0, NULL};
node_t *nil_node=&nil_node_instance;

/************************ DECLARE *************************/

ywMemPool<node_t>             rb_node_pool;
ywMemPool<rb_t>               rb_tree_pool;

void recovery_black_count_balance(rb_t *rbt, node_t *node);
void balanced_rotate(rb_t *rbt, node_t *node, node_side_t side);
bool recover(rb_t *rbt, node_t *node);
void rotate(rb_t *rbt, node_t *node, node_side_t side);
void replace_child(rb_t *rbt, node_t *org_child, node_t *new_child);

/************************ RELATION *************************/
inline rb_t* get_handle(void *rbt) {
    return reinterpret_cast<rb_t*>(rbt);
}
inline node_t *grand_parent(node_t * _node) {
    return _node->parent->parent;
}

inline node_side_t get_side(node_t *_node) {
    if (_node->parent->child[RB_RIGHT] == _node)
        return RB_RIGHT;
    return RB_LEFT;
}

inline node_t *sibling(node_t * _node) {
    return _node->parent->child[1 - get_side(_node)];
}

inline node_t *uncle(node_t *_node) {
    return sibling(_node->parent);
}

inline node_color_t get_color(node_t * _node) {
    return _node->color;
}

inline void replace_child(rb_t *rbt, node_t *org_child, node_t *new_child) {
    if (rbt->root == org_child) {
        rbt->root         = new_child;
        new_child->color  = RB_BLACK;
        new_child->parent = nil_node;
    } else {
        node_t *parent = org_child->parent;

        parent->child[get_side(org_child)] = new_child;
        new_child->parent                  = parent;
    }
}

inline void rotate(rb_t *rbt, node_t *node, node_side_t side) {
    node_t *child = node->child[!side];
    node_t *grandson = child->child[side];

    debug("rotate_%d : root(%d), node(%d), child(%d)\n",
          side,
          rbt->root->key,
          node->key,
          child->key);

    child->child[side] = node;
    replace_child(rbt, node, child);
    node->parent = child;
    node->child[!side] = grandson;
    if (grandson != nil_node) {
        grandson->parent = node;
    }
}

inline node_t *create_node(key_t _key, void * _data) {
    node_t * ret;

    ret = rb_node_pool.alloc();
    if (ret) {
        ret->color  = RB_RED;
        ret->parent = nil_node;
        ret->child[RB_LEFT]= nil_node;
        ret->child[RB_RIGHT]= nil_node;
        ret->key    = _key;
        ret->data   = _data;
    }

    return ret;
}

node_t **__traverse(rb_t *rbt, key_t key, node_t **parent) {
    node_t  **cur = &rbt->root;
    int32_t side;

    *parent = nil_node;
    while (key != (*cur)->key) {
        if (*cur == nil_node) {
            break;
        }
        side = (*cur)->key < key;
        *parent = *cur;
        cur = &(*cur)->child[side];
    }

    return cur;
}

int64_t      rb_get_compare_count() {
    return 0;
}

bool __rb_print(node_t *node, int level) {
    int i;

    if (level >= 160/9) {
        printf("...\n");
        return true;
    }
    if (node != nil_node) {
        printf("%c%-6d- ",
               'R' - ('R'-'B') * get_color(node),
               node->key);
        if (!__rb_print(RIGHT_CHILD(node), level+1)) {
            printf("\n");
        }
        for (i = 0; i <= level; ++i) {
            printf("         ");
        }
        if (!__rb_print(LEFT_CHILD(node), level+1)) {
            printf("\n");
        }
        return true;
    } else {
        printf("N");
        return false;
    }
}

void rb_print(void *_rbt) {
    rb_t *rbt = get_handle(_rbt);

    __rb_print(rbt->root, 0);
}

int32_t __rb_validation(node_t *node) {
    int32_t  child_black_cnt;
    int32_t  cur_black_cnt = 0;
    node_t  *child;
    int      i;

    if (node != nil_node) {
        for (i = 0; i < RB_SIDE_MAX; ++i) {
            child = node->child[i];
            if (child != nil_node) {
                assert((get_color(node) == RB_BLACK) ||
                       (get_color(child) == RB_BLACK));
                assert(child->parent == node);
            }
            child_black_cnt = __rb_validation(child);
            if (cur_black_cnt) {
                assert(child_black_cnt == cur_black_cnt);
            } else {
                cur_black_cnt = child_black_cnt;
            }
        }
        cur_black_cnt += (get_color(node) == RB_BLACK);
    } else {
        cur_black_cnt = 1;
    }

    return cur_black_cnt;
}

void    rb_validation(void * _rbt) {
    rb_t *rbt = get_handle(_rbt);

    (void)__rb_validation(rbt->root);
}

void __rb_infix(node_t *node) {
    if (node != nil_node) {
        assert(get_color(node) == RB_BLACK ||
               get_color(node->parent) == RB_BLACK);
        if (LEFT_CHILD(node) != nil_node) {
            assert(LEFT_CHILD(node)->parent == node);
            __rb_infix(LEFT_CHILD(node));
        }
        printf("%-2d ", node->key);
        if (RIGHT_CHILD(node) != nil_node) {
            assert(RIGHT_CHILD(node)->parent == node);
            __rb_infix(RIGHT_CHILD(node));
        }
    }
}

void rb_infix(void * _rbt) {
    rb_t *rbt = get_handle(_rbt);

    __rb_infix(rbt->root);
}

void *rb_create_tree() {
    rb_t * rbt = rb_tree_pool.alloc();
    rbt->root = nil_node;

    return reinterpret_cast<void*>(rbt);
}

bool rb_insert(void *_rbt, key_t key, void * data) {
    rb_t    *rbt = get_handle(_rbt);
    node_t  *new_node = create_node(key, data);
    node_t **target;
    node_t  *parent;

    if (!new_node) {
        return false;
    }

    if (rbt->root != nil_node) {
        target = __traverse(rbt, key, &parent);
        if (*target != nil_node) {
            rb_node_pool.free_mem(new_node);
            return false;
        }
        new_node->parent = parent;
        *target = new_node;

        if (recover(rbt, new_node)) {
            return true;
        }
        rb_node_pool.free_mem(new_node);
        return false;
    }
    rbt->root = new_node;
    new_node->color = RB_BLACK;

    return true;
}

void        *rb_find(void *_rbt, key_t key) {
    rb_t    *rbt = get_handle(_rbt);
    node_t  *cur = rbt->root;
    int32_t side;

    while (key != cur->key) {
        if (_UNLIKELY(cur == nil_node)) {
            break;
        }
        side = cur->key < key;
        cur = cur->child[side];
    }

    return cur->data;
}

node_t *get_maximum(node_t *node) {
    while (RIGHT_CHILD(node) != nil_node) {
        node = RIGHT_CHILD(node);
    }

    return node;
}

bool rb_remove(void *_rbt, key_t key) {
    rb_t *rbt = get_handle(_rbt);
    if (rbt->root == nil_node) {
        return false;
    }

    node_t *target;
    node_t *child;
    node_t *parent;

    target = *__traverse(rbt, key, &parent);

    if ((target->key != key) ||
        (target == nil_node)) {
        return false;
    }

    if ((LEFT_CHILD(target) != nil_node) &&
        (RIGHT_CHILD(target) != nil_node)) {
        child = get_maximum(LEFT_CHILD(target));
        target->key = child->key;
        target = child;
        child = LEFT_CHILD(target);
    } else {
        child = (LEFT_CHILD(target) != nil_node) ?
                LEFT_CHILD(target) :
                RIGHT_CHILD(target);
    }

    if (get_color(target) == RB_BLACK) {
        recovery_black_count_balance(rbt, target);
    }

    replace_child(rbt, target, child);
    rb_node_pool.free_mem(target);

    return true;
}

void recovery_black_count_balance(rb_t *rbt, node_t *node) {
    node_t      *s_node;
    node_side_t  side = get_side(node);

    /* need not balancing */
    if (node->parent == nil_node) {
        return;
    }

    assert(get_color(node) == RB_BLACK);

    /*unable rebalncing */
    s_node = sibling(node);
    if ((get_color(s_node->parent) == RB_BLACK) &&
        (get_color(s_node) == RB_BLACK) &&
        (get_color(LEFT_CHILD(s_node)) == RB_BLACK) &&
        (get_color(RIGHT_CHILD(s_node)) == RB_BLACK)) {
        s_node->color = RB_RED;
        recovery_black_count_balance(rbt, node->parent);
        return;
    }

    /* make common black */
    if (get_color(s_node->parent) == RB_RED) {
        if ((get_color(LEFT_CHILD(s_node)) == RB_BLACK) &&
           (get_color(RIGHT_CHILD(s_node)) == RB_BLACK)) {
            s_node->color = RB_RED;
            node->parent->color  = RB_BLACK;
            return;
        }
    }

    if (get_color(s_node) == RB_RED) {
        balanced_rotate(rbt, node->parent, !side);
        recovery_black_count_balance(rbt, node);
        return;
    }

    /* rotation*/
    if (get_color(s_node->child[side]) == RB_RED) {
        balanced_rotate(rbt, s_node, side);
        s_node = sibling(node);
    }
    /* bring balck from other side */
    s_node->color = s_node->parent->color;
    node->parent->color = RB_BLACK;
    s_node->child[!side]->color = RB_BLACK;
    rotate(rbt, node->parent, side);

    return;
}

void balanced_rotate(rb_t *rbt, node_t *node, node_side_t side) {
    assert(node->child[side]->color == RB_RED);

    node->child[side]->color = node->color;
    node->color = RB_RED;

    rotate(rbt, node, !side);
}

bool recover(rb_t *rbt, node_t *node) {
    assert(get_color(node) == RB_RED);

    if (rbt->root == node) {
        node->color = RB_BLACK;
        return true;
    }

    /* case2 */
    if (get_color(node->parent) == RB_BLACK)
        return true;

    /* Red - Red - Black */
    assert(node->parent);
    assert(node->parent->parent);
    assert(get_color(grand_parent(node)) == RB_BLACK);

    /* case3 */
    /*    B       R
     *   R R =>  B B
     *  R       R    */
    if (get_color(uncle(node)) == RB_RED) {
        node->parent->color = RB_BLACK;
        uncle(node)->color = RB_BLACK;
        grand_parent(node)->color = RB_RED;
        recover(rbt, grand_parent(node));
        return true;
    }

    /* case4 */
    /*  B    B
     * R  or  R
     *  R    R */
    if (get_side(node) != get_side(node->parent)) {
        rotate(rbt, node->parent, get_side(node->parent));
        node = node->child[get_side(node)];
    }

    /* case5 */
    /*  B       R      B
     * R B =>  B B => R R
     *R       R          B*/
    assert(get_color(uncle(node)) == RB_BLACK);

    balanced_rotate(rbt, grand_parent(node), get_side(node));

    return true;
}

const int32_t  RB_THREAD_COUNT = 4;
const int32_t  RB_TEST_RANGE = 10;
const int32_t  RB_TEST_COUNT = 1024*64;

typedef struct test_arg_struct {
    void     *rbt;
    int32_t   num;
    int32_t   op;
} test_arg;

void rb_test_routine(void * arg) {
    test_arg   *targ = reinterpret_cast<test_arg*>(arg);
    int32_t     i;
    int32_t     j;

    if (targ->op == 0) { /*insert remove */
        for (i = 0; i < RB_TEST_COUNT; ++i) {
            for (j = 0; j < RB_TEST_RANGE; ++j) {
                rb_insert(targ->rbt,  100001 + j*2, NULL);
            }
            for (j = 0; j < RB_TEST_RANGE; ++j) {
                rb_remove(targ->rbt,  100001 + j*2);
            }
        }
    } else {
        for (i = 0; i < RB_TEST_COUNT; ++i) {
            for (j = 0; j < RB_TEST_RANGE; ++j) {
                intptr_t data = reinterpret_cast<intptr_t>(
                    rb_find(targ->rbt,  100000 + j*2));
                assert(data == 100000 + j*2);
            }
        }
    }
}
void rbtree_concunrrency_test(void * rbt) {
    test_arg   targ[RB_THREAD_COUNT];
    int32_t    i;

    for (i = 0; i < RB_TEST_RANGE; ++i) {
        rb_insert(rbt,  100000 + i*2, reinterpret_cast<void*>(100000 + i*2));
    }

    for (i = 0; i< RB_THREAD_COUNT; ++i) {
        targ[i].rbt  = rbt;
        targ[i].num  = i;
        targ[i].op   = 1;
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                rb_test_routine, &targ[i]));
    }
    ywThreadPool::get_instance()->wait_to_idle();

    for (i = 0; i < RB_TEST_RANGE; ++i) {
        rb_remove(rbt,  100000 + i*2);
    }
}
