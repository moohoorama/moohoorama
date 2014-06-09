/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSKIPLIST_H_
#define INCLUDE_YWSKIPLIST_H_

#include <ywcommon.h>
#include <ywaccumulator.h>
#include <ywspinlock.h>

typedef struct ywskNodeStruct      ywskNode;
typedef uint32_t ywKeyType;

struct ywskNodeStruct {
    ywskNodeStruct():next(NULL), down(NULL) {
    }
    ywskNode   *next;
    ywskNode   *down;
    ywskNode   *up;
    ywKeyType   key;
    ywSpinLock  lock;
};

template<typename T, int32_t SIZE>
class  ywSimpleStack {
 public:
    ywSimpleStack() {
        cnt = 0;
    }

    bool push(T _new) {
        if (cnt < SIZE) {
            data[cnt++] = _new;
            return true;
        }

        return false;
    }
    bool pop(T *_ret) {
        if (cnt) {
            *_ret = data[--cnt];
            return true;
        }

        return false;
    }

 private:
    int32_t     cnt;
    T           data[SIZE];
};

class ywSkipList {
 public:
    static const int32_t   DEGREE    = 3;
    static const int32_t   MAX_LEVEL = 64/(1 << DEGREE);
    static const ywKeyType INIT_KEY  = 0;
    static const ywKeyType LAST_KEY  = UINT32_MAX;

    static const int32_t getDegree() {
        return 1 << DEGREE;
    }

    explicit ywSkipList():level(MAX_LEVEL), seq(0) {
        init();
    }
    explicit ywSkipList(int32_t _level):level(_level), seq(0) {
        init();
    }

    ~ywSkipList() {
        // TODO(ywkim) destroy all
    }

    ywskNode *begin() {
        return heads[level-1].next;
    }
    ywskNode *end() {
        return &tails[level-1];
    }
    void fetchNext(ywskNode ** cursor) {
        (*cursor) = (*cursor)->next;
    }

    void validate() {
#ifdef DEBUG
        ywskNode  *cursor;
        ywKeyType  prev_key;

        prev_key = INIT_KEY;
        for (cursor = begin(); cursor != end(); fetchNext(&cursor)) {
            assert(prev_key < cursor->key);
            prev_key = cursor->key;
        }
#endif
    }

    bool insert(ywKeyType target) {
        ywskNode      *traverse_history[MAX_LEVEL];
        int32_t        new_node_level = get_new_level();
        ywskNode      *new_node;
        ywskNode      *down_node;
        ywskNode      *base_node;
        int32_t        i;
        ywSimpleStack<ywSpinLock*, MAX_LEVEL*2> lockStack;

        if (target == INIT_KEY) {
            return false;
        }

        while (true) {
            if (_search(target, traverse_history)->next->key <= target) {
                return true; /*dupkey*/
            }
            for (i = 1; i <=new_node_level; ++i) {
                base_node = traverse_history[level - i];
                if (!lock(base_node, &lockStack) ||
                    base_node->next->key <= target ) {
                    unlock(&lockStack);
                    break;
                }
            }
            if (i == new_node_level+ 1) {
                break;
            }
        }

        /* bottom_up! */
        down_node = NULL;
        for (i = 1; i <=new_node_level; ++i) {
            base_node = traverse_history[ level - i];

            new_node       = new ywskNode();
            new_node->key  = target;
            new_node->down = down_node;
            new_node->up   = NULL;
            if (down_node) {
                down_node->up = new_node;
            }
            new_node->next = base_node->next;
            down_node = new_node;

            base_node->next = new_node;
        }
        unlock(&lockStack);
        node_count.mutate(new_node_level);
        key_count.mutate(1);

        return true;
    }

    bool remove(ywKeyType target) {
        ywskNode  *traverse_history[MAX_LEVEL];
        ywskNode  *base_node;
        ywskNode  *rem_node;
        int32_t   depth = 0;
        int32_t   i;
        ywSimpleStack<ywSpinLock*, MAX_LEVEL*2> lockStack;

        if (target == INIT_KEY) {
            return false;
        }

        do {
            base_node = _search(target, traverse_history);
            rem_node = base_node->next;
            if (rem_node->key != target) {
                /* try pessimistic */
//                return false; /*Cannot find*/
                continue;
            }

            for (i = 1; i <=level; ++i) {
                base_node = traverse_history[level - i];
                if (!lock(base_node, &lockStack) ||
                    !lock(rem_node, &lockStack) ||
                    base_node->next != rem_node) {
                    unlock(&lockStack);
                    break;
                }
                rem_node = rem_node->up;
                if (rem_node == NULL) {
                    depth = i;
                    break;
                }
            }
        } while (!depth);

        /* bottom_up! */
        for (i = 1; i <= depth; ++i) {
            base_node = traverse_history[level - i];
            rem_node = base_node->next;

            assert(rem_node->key == target);
            assert(base_node->lock.hasWLock());
            assert(rem_node->lock.hasWLock());

            base_node->next = rem_node->next;
            // TODO(ywkim)  link_remove_list(rem_node);
        }

        assert(rem_node->up == NULL);
        unlock(&lockStack);

        key_count.mutate(-1);
        node_count.mutate(-depth);

        return true;
    }

    void print() {
        ywskNode  *traverse_history[MAX_LEVEL];
        ywKeyType  curKey;
        int32_t    i;

        /* init */
        for (i = 0; i < level; ++i) {
            traverse_history[i]=heads[i].next;
        }
        while (traverse_history[level-1] != &tails[level-1]) {
            curKey = traverse_history[level-1]->key;
            for (i = 1; i <= level; ++i) {
                if (traverse_history[level-i]->key != curKey) {
                    break;
                }
                printf("%16d ", traverse_history[level-i]->key);
                traverse_history[level-i] = traverse_history[level-i]->next;
            }
            printf("\n");
        }
        print_stat();
    }

    void print_stat() {
        printf("node_count    : %" PRIu64 "\n", node_count.sum());
        printf("key_count     : %" PRIu64 "\n", key_count.sum());
        printf("next_trav     : %" PRIu64 "\n", next_trav.sum());
        printf("down_trav     : %" PRIu64 "\n", down_trav.sum());
        if (down_trav.sum()) {
            printf("search_ratio  : %f\n",
                   static_cast<float>(next_trav.sum())/down_trav.sum());
        }
        printf("\n");
    }

    int32_t  get_new_level() {
        uint64_t i;
        uint64_t mask;
        ++seq;
        for (i = level-1; i > 0; --i) {
            mask = (1 << i*DEGREE) - 1;
            if ((seq & mask) == mask) {
                return i+1;
            }
        }
        return 1;
    }

    ywskNode *search(ywKeyType target) {
        ywskNode  *traverse_history[MAX_LEVEL];
        return _search(target, traverse_history)->next;
    }

    uint64_t get_key_count() {
        return key_count.sum();
    }

    uint64_t get_node_count() {
        return node_count.sum();
    }

 private:
    void init() {
        int32_t i;

        assert(level <= MAX_LEVEL);

        heads = new ywskNode[level];
        tails = new ywskNode[level];
        for (i = 0; i < level; ++i) {
            heads[i].next = &tails[i];
            heads[i].key  = INIT_KEY;
            heads[i].down = &heads[i+1];
            heads[i].up   = &heads[i-1];

            tails[i].next = &heads[i];
            tails[i].key  = LAST_KEY;
            tails[i].down = &tails[i+1];
            tails[i].up   = &tails[i-1];
        }
        heads[0].up = NULL;
        tails[0].up = NULL;
        heads[level-1].down = NULL;
        tails[level-1].down = NULL;
    }

    bool lock(ywskNode                             *target,
              ywSimpleStack<ywSpinLock*, MAX_LEVEL*2> *lockStack) {
        if (target->lock.WLock()) {
            assert(lockStack->push(&(target->lock)));
            return true;
        }
        return false;
    }
    void unlock(ywSimpleStack<ywSpinLock*, MAX_LEVEL*2> *lockStack) {
        ywSpinLock *lock;
        while (lockStack->pop(&lock)) {
            lock->release();
        }
    }

    ywskNode *_search(ywKeyType target, ywskNode *traverse_history[]) {
        int32_t   level_cursor = 0;
        ywskNode * cursor = &heads[0];
        int32_t   _next_trav = 0;

        while (true) {
            if (cursor->next->key < target) {
                cursor = cursor->next;
                ++_next_trav;
            } else {
                traverse_history[level_cursor] = cursor;
                level_cursor++;
                if (level_cursor >= level) {
                    next_trav.mutate(_next_trav);
                    down_trav.mutate(level);
                    return cursor;
                }
                cursor = cursor->down;
            }
        }
        assert(false);

        return NULL;
    }

    ywskNode *heads;
    ywskNode *tails;
    int32_t   level;
    uint64_t  seq;
    ywAccumulator<int64_t, false> key_count;
    ywAccumulator<int64_t, false> node_count;
    ywAccumulator<int64_t, false> next_trav;
    ywAccumulator<int64_t, false> down_trav;
};

extern void skiplist_level_test();
extern void skiplist_insert_remove_test();
extern void skiplist_map_insert_perf_test();
extern void skiplist_insert_perf_test();
extern void skiplist_conc_insert_test();
extern void skiplist_conc_remove_test();

#endif  // INCLUDE_YWSKIPLIST_H_
