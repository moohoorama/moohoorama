/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrbtree.h>
#include <ywthread.h>
#include <ywaccumulator.h>
#include <gtest/gtest.h>

#define LEFT_CHILD(node) ((node)->child[RB_LEFT])
#define RIGHT_CHILD(node) ((node)->child[RB_RIGHT])
#define debug(...)

node_t nil_node_instance={RB_BLACK, &nil_node_instance,
    {&nil_node_instance, &nil_node_instance}, 0};
node_t *nil_node=&nil_node_instance;

/************************ DECLARE *************************/
node_t      *grand_parent(node_t * _node);
node_t      *right(node_t *_node);
node_side_t  get_side(node_t *_node);
node_t      *sibling(node_t * _node);
node_t      *uncle(node_t *_node);
node_color_t get_color(node_t * _node);

void         recovery_black_count_balance(node_t **root, node_t *node);
void         balanced_rotate(node_t **root, node_t *node, node_side_t side);
bool         recover(node_t **root, node_t *node);

void         rotate(node_t **root, node_t *node, node_side_t side);

void         replace_child(node_t **root, node_t *org_child, node_t *new_child);

node_t      *create_node(key_t _key);
node_t     **__traverse(node_t **root, key_t key, node_t **parent);
bool         recover(node_t **root, node_t *node);

ywAccumulator<int64_t, false> compare_count;

/************************ RELATION *************************/
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

inline void replace_child(node_t **root, node_t *org_child, node_t *new_child) {
    if (*root == org_child) {
        *root             = new_child;
        new_child->color  = RB_BLACK;
        new_child->parent = nil_node;
    } else {
        node_t *parent = org_child->parent;

        parent->child[get_side(org_child)] = new_child;
        new_child->parent                  = parent;
    }
}

inline void rotate(node_t **root, node_t *node, node_side_t side) {
    node_t *child = node->child[!side];
    node_t *grandson = child->child[side];

    debug("rotate_%d : root(%d), node(%d), child(%d)\n",
          side,
          (*root)->key,
          node->key,
          child->key);

    child->child[side] = node;
    replace_child(root, node, child);
    node->parent = child;
    node->child[!side] = grandson;
    if (grandson != nil_node) {
        grandson->parent = node;
    }
}

inline node_t *create_node(key_t _key) {
    node_t * ret;

    ret = new node_t();
    ret->color  = RB_RED;
    ret->parent = nil_node;
    ret->child[RB_LEFT]= nil_node;
    ret->child[RB_RIGHT]= nil_node;
    ret->key    = _key;

    return ret;
}

node_t **__traverse(node_t **root, key_t key, node_t **parent) {
    node_t  **cur = root;
    int32_t side;
    int32_t _compare_count = 0;

    *parent = nil_node;
    while (key != (*cur)->key) {
        if (*cur == nil_node) {
            break;
        }
        side = (*cur)->key < key;
        ++_compare_count;
        *parent = *cur;
        cur = &(*cur)->child[side];
    }

    compare_count.mutate(_compare_count);
    return cur;
}

int64_t      rb_get_compare_count() {
    return compare_count.get();
}

bool rb_print(int level, node_t *node) {
    int i;

    if (level >= 160/9) {
        printf("...\n");
        return true;
    }
    if (node != nil_node) {
        printf("%c%-6d- ",
               'R' - ('R'-'B') * get_color(node),
               node->key);
        if (!rb_print(level+1, RIGHT_CHILD(node))) {
            printf("\n");
        }
        for (i = 0; i <= level; ++i) {
            printf("         ");
        }
        if (!rb_print(level+1, LEFT_CHILD(node))) {
            printf("\n");
        }
        return true;
    } else {
        printf("N");
        return false;
    }
}


int32_t rb_validation(node_t *node) {
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
            child_black_cnt = rb_validation(child);
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

void rb_infix(node_t *node) {
    if (node != nil_node) {
        assert(get_color(node) == RB_BLACK ||
               get_color(node->parent) == RB_BLACK);
        if (LEFT_CHILD(node) != nil_node) {
            assert(LEFT_CHILD(node)->parent == node);
            rb_infix(LEFT_CHILD(node));
        }
        printf("%-2d ", node->key);
        if (RIGHT_CHILD(node) != nil_node) {
            assert(RIGHT_CHILD(node)->parent == node);
            rb_infix(RIGHT_CHILD(node));
        }
    }
}

node_t      *rb_create_tree() {
    return nil_node;
}

bool rb_insert(node_t **root, key_t key) {
    node_t  *new_node = create_node(key);
    node_t **target;
    node_t  *parent;

    if (*root != nil_node) {
        target = __traverse(root, key, &parent);
        if (*target != nil_node) {
            return false;
        }
        new_node->parent = parent;
        *target = new_node;

        if (recover(root, new_node)) {
            return true;
        }
        return false;
    }
    *root = new_node;
    new_node->color = RB_BLACK;

    return true;
}

node_t      *rb_find(node_t **root, key_t key) {
    node_t **target;
    node_t  *parent;

    target = __traverse(root, key, &parent);
    return *target;
}

node_t *get_maximum(node_t *node) {
    while (RIGHT_CHILD(node) != nil_node) {
        node = RIGHT_CHILD(node);
    }

    return node;
}

bool rb_remove(node_t **root, key_t key) {
    if (*root == nil_node) {
        return false;
    }

    node_t *target;
    node_t *child;
    node_t *parent;

    target = *__traverse(root, key, &parent);

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
        recovery_black_count_balance(root, target);
    }

    replace_child(root, target, child);
    delete target;

    return true;
}

void recovery_black_count_balance(node_t **root, node_t *node) {
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
        recovery_black_count_balance(root, node->parent);
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
        balanced_rotate(root, node->parent, !side);
        recovery_black_count_balance(root, node);
        return;
    }

    /* rotation*/
    if (get_color(s_node->child[side]) == RB_RED) {
        balanced_rotate(root, s_node, side);
        s_node = sibling(node);
    }
    /* bring balck from other side */
    s_node->color = s_node->parent->color;
    node->parent->color = RB_BLACK;
    s_node->child[!side]->color = RB_BLACK;
    rotate(root, node->parent, side);

    return;
}

void balanced_rotate(node_t **root, node_t *node, node_side_t side) {
    assert(node->child[side]->color == RB_RED);

    node->child[side]->color = node->color;
    node->color = RB_RED;

    rotate(root, node, !side);
}

bool recover(node_t **root, node_t *node) {
    assert(get_color(node) == RB_RED);

    if (*root == node) {
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
        recover(root, grand_parent(node));
        return true;
    }

    /* case4 */
    /*  B    B
     * R  or  R
     *  R    R */
    if (get_side(node) != get_side(node->parent)) {
        rotate(root, node->parent, get_side(node->parent));
        node = node->child[get_side(node)];
    }

    /* case5 */
    /*  B       R      B
     * R B =>  B B => R R
     *R       R          B*/
    assert(get_color(uncle(node)) == RB_BLACK);

    balanced_rotate(root, grand_parent(node), get_side(node));

    return true;
}

const int32_t  RB_THREAD_COUNT = 4;
const int32_t  RB_TEST_COUNT = 1024*32;

typedef struct test_arg_struct {
    node_t  **root;
    int32_t   num;
    int32_t   op;
} test_arg;

void rb_test_routine(void * arg) {
    test_arg   *targ = reinterpret_cast<test_arg*>(arg);
    int32_t     i;

    if (targ->op == 0) { /*insert remove */
        for (i = 0; i < RB_TEST_COUNT; ++i) {
            rb_insert(targ->root,   80008);
            rb_remove(targ->root,   80008);
            rb_insert(targ->root,  100008);
            rb_remove(targ->root,  100008);
            rb_insert(targ->root,  480008);
            rb_remove(targ->root,  480008);
            rb_insert(targ->root,  520008);
            rb_remove(targ->root,  520008);
            rb_insert(targ->root, 1080008);
            rb_remove(targ->root, 1080008);
            rb_insert(targ->root, 1020008);
            rb_remove(targ->root, 1020008);
        }
        printf("Mutate done.\n");
    } else {
        for (i = 0; i < RB_TEST_COUNT*10; ++i) {
            ASSERT_TRUE(rb_find(targ->root,  100000));
            ASSERT_TRUE(rb_find(targ->root,  500000));
            ASSERT_TRUE(rb_find(targ->root, 1000000));
        }
        printf("Find done.\n");
    }
}
void rbtree_concunrrency_test(node_t **root) {
    test_arg   targ[RB_THREAD_COUNT];
    int32_t    i;

    rb_insert(root,  100000);
    rb_insert(root,  500000);
    rb_insert(root, 1000000);

    printf("CompareCount : %lld\n", rb_get_compare_count());
    printf("try_count    : %d\n", RB_TEST_COUNT*10);
    printf("AVG_COMPARE  : %lld\n",
           rb_get_compare_count()/(
               RB_TEST_COUNT*10*(RB_THREAD_COUNT)));

    for (i = 0; i< RB_THREAD_COUNT; ++i) {
        targ[i].root = root;
        targ[i].num  = i;
        targ[i].op   = 1;
        ASSERT_TRUE(ywThreadPool::get_instance()->add_task(
                rb_test_routine, &targ[i]));
    }

    ywThreadPool::get_instance()->wait_to_idle();

    printf("CompareCount : %lld\n", rb_get_compare_count());
    printf("try_count    : %d\n", RB_TEST_COUNT*10);
    printf("AVG_COMPARE  : %lld\n",
           rb_get_compare_count()/(
               RB_TEST_COUNT*10*(RB_THREAD_COUNT)));
}
