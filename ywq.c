#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include "ywq.h"

void   ywqInit( struct ywq * _head )
{
    _head->prev = _head;
    _head->next = _head;
    _head->data = NULL;
    _head->meta = 9999;
}
void   ywqPush( struct ywq * _head, struct ywq * _target )
{
    struct ywq * oldPrev;
    do
    {
        oldPrev = _head->prev;
        _target->prev = oldPrev;
        _target->next = _head;

    } while( !__sync_bool_compare_and_swap( &_head->prev, oldPrev, _target ) );

    __sync_synchronize();

    oldPrev->next = _target;
}
struct ywq  * ywqPop( struct ywq * _head )
{
    struct ywq * oldNext = NULL;
    struct ywq * oldNextNext;

    do
    {
        oldNext     = _head->next;
        if( oldNext == _head ) return NULL;
        oldNextNext = oldNext->next;
        if( oldNextNext->prev != oldNext ) return NULL;
    } while( !__sync_bool_compare_and_swap( &_head->next, oldNext, oldNextNext ) );

    return (struct ywq*)oldNext;
}

static struct ywq head;
static struct ywq slot[1024];

void printList()
{
    struct ywq * iter;

    YWQ_FOREACH( iter, &head )
    {
        printf("[%8x %8x %5d %8x]\n", 
                (unsigned int)iter->prev, 
                (unsigned int)iter, iter->meta,
                (unsigned int)iter->next );
    }
    printf("\n");
}

void * route( void * arg )
{
    int          num = (int)arg;
    struct ywq * iter;
    int          i;

    if( num == 0 )
    {
        for( i = 0 ; i < 1024 ; i ++ )
        {
            slot[i].meta = i + 100000;
            ywqPush( &head, &slot[i] );
        }
    }
    else
    {
        for( i = 0 ; i < 1024 ; i ++ )
        {
            while( !(iter = ywqPop( &head )) );
        }
    }

    return (void*)num;
}

void   ywqTest()
{
    pthread_t    pt[2];
    int          j;

    ywqInit( &head );

    assert( 0 == pthread_create( &pt[1], NULL/*attr*/, route, (void*)1 ) );
    assert( 0 == pthread_create( &pt[0], NULL/*attr*/, route, (void*)0 ) );
    pthread_join( pt[0],(void**)&j );
    pthread_join( pt[1],(void**)&j );
    printList();
    printf("\n");
}
