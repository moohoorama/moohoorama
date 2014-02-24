/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywrbtree.h>

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

void         rotate_left(node_t **root, node_t *node);
void         rotate_right(node_t **root, node_t *node);

void         replace_child(node_t **root, node_t *org_child, node_t *new_child);

node_t      *create_node(key_t _key);
int32_t      compare(node_t *node, key_t key);
node_t     **__traverse(node_t **root, key_t key, node_t **parent);
bool         recover(node_t **root, node_t *node);

static       int32_t compare_count = 0;

/************************ RELATION *************************/
inline node_t *grand_parent(node_t * _node) {
    if (_node->parent != nil_node)
        return _node->parent->parent;
    return nil_node;
}

inline node_side_t get_side(node_t *_node) {
    if (_node->parent != nil_node)
        if (_node->parent->child[RB_RIGHT] == _node)
            return RB_RIGHT;
    return RB_LEFT;
}

inline node_t *sibling(node_t * _node) {
    if (_node->parent != nil_node)
        return _node->parent->child[1 - get_side(_node)];
    return nil_node;
}

inline node_t *uncle(node_t *_node) {
    if (_node->parent != nil_node) {
        return sibling(_node->parent);
    }
    return nil_node;
}

inline node_color_t get_color(node_t * _node) {
    if (_node == nil_node) {
        return RB_BLACK;
    }
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

/*     P             P     
 *    @             @    
 *   N             C   
 *  . @    =>     @ .  
 * 1   C         N   3 
 *    @ .       . @    
 *   2   3     1   2       */
inline void rotate_left(node_t **root, node_t *node) {
    node_t *child  = RIGHT_CHILD(node);
    node_t *node_2 = LEFT_CHILD(child);

    debug("rotate_left : root(%d), node(%d), child(%d)\n",
           (*root)->key,
           node->key,
           child->key);

    replace_child(root, node, child);
    LEFT_CHILD(child)  = node;
    node->parent       = child;
    RIGHT_CHILD(node)  = node_2;
    if (node_2 != nil_node) {
        node_2->parent     = node;
    }
}

/*   P          P     
 *    @          @    
 *     N          C   
 *    @ .   =>   . @  
 *   C   3      1   N 
 *  . @            @ .   
 * 1   2          2   3 */
inline void rotate_right(node_t **root, node_t *node) {
    node_t *child  = LEFT_CHILD(node);
    node_t *node_2 = RIGHT_CHILD(child);

    debug("rotate_right : root(%d), node(%d), child(%d)\n",
           (*root)->key,
           node->key,
           child->key);

    replace_child(root, node, child);
    RIGHT_CHILD(child) = node;
    node->parent       = child;
    LEFT_CHILD(node)   = node_2;
    if (node_2 != nil_node) {
        node_2->parent     = node;
    }
}

node_t *create_node(key_t _key) {
    node_t * ret;

    ret = new node_t();
    ret->color  = RB_RED;
    ret->parent = nil_node;
    ret->child[RB_LEFT]= nil_node;
    ret->child[RB_RIGHT]= nil_node;
    ret->key    = _key;

    return ret;
}

inline int32_t compare(node_t *node, key_t key) {
    if (node == nil_node)
        return 0;
    return key - node->key;
}

#define NORMAL

inline int32_t get_sign(int32_t ret) {
    return static_cast<uint32_t>(ret)>>31;
}

node_t **__traverse(node_t **root, key_t key, node_t **parent) {
    node_t  **cur = root;
    int32_t ret;

#ifdef NORMAL
    *parent = nil_node;
    while ((ret = compare(*cur, key))) {
        ++compare_count;
        *parent = *cur;
        if (ret < 0) {
            cur = &LEFT_CHILD(*cur);
        } else {
            cur = &RIGHT_CHILD(*cur);
        }
    }
#else
    int32_t sign;

    *parent = nil_node;
    while (*cur != nil_node) {
        *parent = *cur;
        ret = (*cur)->key - key;
        sign = get_sign(ret);
        cur = &(*cur)->child[sign];
    }
#endif

    return cur;
}

int32_t      rb_get_compare_count() {
    return compare_count;
}

bool rb_print(int level, node_t *node) {
    int i;

    if (level >= 160/7) {
        printf("...\n");
        return true;
    }
    if (node != nil_node) {
        printf("%c%-4d- ",
               'R' - ('R'-'B') * get_color(node),
               node->key);
        if (!rb_print(level+1, RIGHT_CHILD(node))) {
            printf("\n");
        }
        for (i = 0; i <= level; ++i) {
            printf("       ");
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
        (target != nil_node)) {
        return false;
    }

    if ((LEFT_CHILD(target) != nil_node) &&
        (RIGHT_CHILD(target) != nil_node)) {
        child = get_maximum(LEFT_CHILD(target));
        target->key = child->key;
        target = child;
    }
    assert((LEFT_CHILD(target) == nil_node) ||
           (RIGHT_CHILD(target) == nil_node));

    child = (LEFT_CHILD(target) != nil_node) ?
            LEFT_CHILD(target) :
            RIGHT_CHILD(target);

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
    s_node->color = s_node->parent->color;
    node->parent->color = RB_BLACK;
    s_node->child[!side]->color = RB_BLACK;
    if (side == RB_LEFT) {
        rotate_left(root, node->parent);
    } else {
        rotate_right(root, node->parent);
    }

    return;
}

void balanced_rotate(node_t **root, node_t *node, node_side_t side) {
    assert(node->child[side]->color == RB_RED);

    node->child[side]->color = node->color;
    node->color = RB_RED;

    if (side == RB_LEFT) {
        rotate_right(root, node);
    } else {
        rotate_left(root, node);
    }
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
    if (get_side(node) == RB_LEFT) {
        if (get_side(node->parent) == RB_RIGHT) {
            rotate_right(root, node->parent);
            node = RIGHT_CHILD(node);
        }
    } else { /* right */
        if (get_side(node->parent) == RB_LEFT) {
            rotate_left(root, node->parent);
            node = LEFT_CHILD(node);
        }
    }

    /* case5 */
    /*  B       R      B
     * R B =>  B B => R R
     *R       R          B*/
    assert(get_color(uncle(node)) == RB_BLACK);

    balanced_rotate(root, grand_parent(node), get_side(node));
    /*
    node->parent->color       = RB_BLACK;
    grand_parent(node)->color = RB_RED;
    if (get_side(node) == RB_LEFT) {
        rotate_right(root, grand_parent(node));
    } else {
        rotate_left(root, grand_parent(node));
    }
    */

    return true;
}

