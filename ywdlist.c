#include <stdio.h>

#include "ywdlist.h"

#define YWDL_LINK( c, n ) if(c) (c)->next = (n); if(n) (n)->prev=(c);

void dlistInit( struct ywDList * _head )
{
    _head->prev = NULL;
    _head->next = NULL;
    _head->data = NULL;
    _head->meta = 0;
}

void dlistAttach( struct ywDList * _head, struct ywDList * target )
{
    YWDL_LINK( target, _head->next );
    YWDL_LINK( _head, target );
}

void dlistDetach( struct ywDList * _head, struct ywDList * target )
{
    YWDL_LINK( target->prev, target->next );
}

void dlistTest()
{
    struct ywDList   head;
    struct ywDList   slot[16];
    struct ywDList * iter;
    struct ywDList * iter2;
    int              i;

    dlistInit( &head );

    YWD_FOREACH( iter, &head )
        printf("%2d ", iter->meta );
    printf("\n");

    for( i = 0 ; i < 4 ; i ++ )
    {
        slot[i].meta = i+10;
        dlistAttach( &head, &slot[i] );

        YWD_FOREACH( iter2, &head )
            printf("%2d ", iter2->meta );
        printf("[%d]\n",i);
    }

    YWD_FOREACH( iter, &head )
    {
        dlistDetach( &head, iter );

        YWD_FOREACH( iter2, &head )
            printf("%2d ", iter2->meta );
        printf("\n");
    }
}

