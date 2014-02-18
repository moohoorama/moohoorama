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
    static const uint32_t MAX_LEVEL = 32/(1 << DEGREE);
    static const keyType  INIT_KEY  = 0;

    static const uint32_t getDegree() {
        return 1 << DEGREE;
    }

    explicit ywSkipList():level(MAX_LEVEL), key_count(0), node_count(0),
        next_trav(0), down_trav(0) {
        init();
    }
    explicit ywSkipList(uint32_t _level):
        level(_level), key_count(0), node_count(0),
        next_trav(0), down_trav(0) {
        init();
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

            ++node_count;
            new_node       = new ywskNode();
            new_node->key  = target;
            new_node->down = down_node;
            down_node = new_node;

            new_node->next  = prev_node->next;
            prev_node->next = new_node;
        }

        ++key_count;
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

            --node_count;
            delete free_node;
        }
        if (i != 1) {
            --key_count;
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
        printf("node_count : %d\n", node_count);
        printf("key_count  : %d\n", key_count);
        printf("next_trav  : %"PRIu64"\n", next_trav);
        printf("down_trav  : %"PRIu64"\n", down_trav);
        printf("\n");
    }
    uint32_t  get_new_level() {
        static uint32_t seq = 0;
        uint32_t i;
        uint32_t mask;
        ++seq;
        for (i = level-1; i > 0; --i) {
            mask = (1 << i*DEGREE) - 1;
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
//                ++next_trav;
            } else {
//                ++down_trav;
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
    uint64_t  next_trav;
    uint64_t  down_trav;
};

#endif  // INCLUDE_YWSKIPLIST_H_
