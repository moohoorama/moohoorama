#ifndef __YWDLIST_H__
#define __YWDLIST_H__

#include <stdint.h>

#define YWD_FOREACH(i,h) for( (i) = (h)->next ; (i) != NULL ; (i) = (i)->next )

struct ywDList
{
    struct ywDList   * prev;
    struct ywDList   * next;
    void             * data;
    uint32_t           meta;
};

void dlistInit( struct ywDList * _head );
void dlistAttach( struct ywDList * _head, struct ywDList * target );
void dlistDetach( struct ywDList * _head, struct ywDList * target );

void dlistTest();

#endif  /*__YWDLIST_H__*/
