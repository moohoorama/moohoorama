/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSKIPLIST_H_
#define INCLUDE_YWSKIPLIST_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <assert.h>
#include <string.h>

typedef struct ywskNodeStruct ywskNode;
typedef uint32_t keyType;

struct ywskNodeStruct {
    ywskNodeStruct():next(NULL), down(NULL) {
    }
    ywskNode * next;
    ywskNode * down;
    keyType    key;
};

class ywSkipList {
 public:
    static const uint32_t DEGREE    = 3;
    static const uint32_t MAX_LEVEL = 64/(1 << DEGREE);
    static const keyType  INIT_KEY  = 0;

    static const uint32_t getDegree() {
        return 1 << DEGREE;
    }

    explicit ywSkipList():level(MAX_LEVEL), key_count(0), node_count(0),
        seq(0), next_trav(0), down_trav(0) {
        init();
    }
    explicit ywSkipList(uint32_t _level):
        level(_level), key_count(0), node_count(0),
        seq(0), next_trav(0), down_trav(0) {
        init();
    }

    ywskNode *begin() {
        return &heads[level-1];
    }
    ywskNode *end() {
        return &heads[level-1];
    }
    ywskNode *fetchNext(ywskNode * cursor) {
        return cursor->next;
    }


    bool insert(keyType target) {
        ywskNode  *traverse_history[MAX_LEVEL];
        uint32_t   new_node_level = get_new_level();
        ywskNode  *new_node;
        ywskNode  *prev_node;
        ywskNode  *down_node;
        keyType    found_key;
        uint32_t   i;

        if (target == INIT_KEY) {
            return false;
        }

        found_key = _search(target, traverse_history)->next->key;
        if (found_key == target) {
            return true; /*dupkey*/
        }

        /* bottom_up! */
        down_node = NULL;
        for (i = 1; i <=new_node_level; ++i) {
            prev_node = traverse_history[ level - i];

            new_node       = new ywskNode();
            new_node->key  = target;
            new_node->down = down_node;
            down_node = new_node;

            do {
                new_node->next  = prev_node->next;
            } while ( !__sync_bool_compare_and_swap(
                    &prev_node->next, new_node->next, new_node));
//            __sync_add_and_fetch(&node_count, 1);
        }

//        __sync_add_and_fetch(&key_count, 1);
        return true;
    }

    bool remove(keyType target) {
        ywskNode  *traverse_history[MAX_LEVEL];
        ywskNode  *target_node;
        ywskNode  *free_node;
        uint32_t   i;

        if (target == INIT_KEY) {
            return false;
        }

        (void) _search(target, traverse_history);

        /* bottom_up! */
        for (i = 1; i <=level; ++i) {
            target_node = traverse_history[ level - i];
            if (target_node->next->key != target) {
                break;
            }
            free_node         = target_node->next;
            target_node->next = free_node->next;

            __sync_add_and_fetch(&node_count, -1);
            delete free_node;
        }
        if (i != 1) {
            __sync_add_and_fetch(&key_count, -1);
        }

        return true;
    }

    void print() {
        ywskNode  *traverse_history[MAX_LEVEL];
        uint32_t   i;
        keyType    curKey;

        /* init */
        for (i = 0; i < level; ++i) {
            traverse_history[i]=heads[i].next;
        }
        while (traverse_history[level-1] != &heads[level-1]) {
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
        printf("node_count    : %d\n", node_count);
        printf("key_count     : %d\n", key_count);
        printf("next_trav     : %"PRIu64"\n", next_trav);
        printf("down_trav     : %"PRIu64"\n", down_trav);
        if (down_trav) {
            printf("search_ratio  : %f\n",
                   static_cast<float>(next_trav)/down_trav);
        }
        printf("\n");
    }

    uint32_t  get_new_level() {
        uint64_t i;
        uint64_t mask;
        ++seq;
        for (i = level-1; i > 0; --i) {
            mask = static_cast<uint64_t>((1 << i*DEGREE) - 1);
            if ((seq & mask) == mask) {
                return i+1;
            }
        }
        return 1;
    }

    ywskNode *search(keyType target) {
        ywskNode  *traverse_history[MAX_LEVEL];
        return _search(target, traverse_history);
    }

    uint32_t get_key_count() {
        return key_count;
    }

    uint32_t get_node_count() {
        return node_count;
    }

 private:
    void init() {
        uint32_t i;

        assert(level <= MAX_LEVEL);

        heads = new ywskNode[level];
        for (i = 0; i < level; ++i) {
            heads[i].next = &heads[i];
            heads[i].key  = INIT_KEY;
            heads[i].down = &heads[i+1];
        }
        heads[level-1].down = NULL;
    }

    ywskNode *_search(keyType target, ywskNode *traverse_history[]) {
        uint32_t   level_cursor = 0;
        ywskNode * cursor = &heads[0];

        while (true) {
            if (!isLast(cursor->next, level_cursor) &&
                cursor->next->key < target) {
                cursor = cursor->next;
                ++next_trav;
            } else {
                ++down_trav;
                traverse_history[level_cursor] = cursor;
                level_cursor++;
                if (level_cursor >= level) {
                    return cursor;
                }
                cursor = cursor->down;
            }
        }
        assert(false);

        return NULL;
    }

    bool isLast(ywskNode *node, int level) {
        return node == &(heads[level]);
    }

    ywskNode *heads;
    uint32_t  level;
    uint32_t  key_count;
    uint32_t  node_count;
    uint64_t  seq;
    uint64_t  next_trav;
    uint64_t  down_trav;
};

#endif  // INCLUDE_YWSKIPLIST_H_
