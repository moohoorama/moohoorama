#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include "ywq.h"

void printList();
void   ywqInit( struct ywq * _head )
{
    _head->prev = _head;
    _head->next = _head;
    _head->data = NULL;
    _head->meta = 9999;
    _head->size = 0;
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
    __sync_add_and_fetch( &_head->size, 1 );

    oldPrev->next = _target;
}
static struct ywq dummy;
struct ywq  * ywqPop( struct ywq * _head )
{
    struct ywq * oldNext = NULL;
    struct ywq * oldNextNext;
    int          tryCount = 0;

    do {
        do {
            for(;;) {
                tryCount ++;
                oldNext     = _head->next;
                if( oldNext == _head ) return NULL;
                oldNextNext = oldNext->next;
                __sync_synchronize();
                if( ( oldNextNext != _head ) ||
                    ( oldNext == &dummy ) ) break;
                dummy.prev=dummy.next=NULL;
                dummy.meta=1;
                assert( dummy.size == 0 );
                __sync_add_and_fetch( &dummy.size, 1 );
                ywqPush( _head, &dummy );
            }
            if( oldNextNext->prev != oldNext )
            {
                printf("        %d %d\n",oldNext->prev->meta,oldNextNext->prev->meta );
                printf("        %d %d\n",oldNext->meta,oldNextNext->meta );
                printf("    %d\n",dummy.size);
                printList();
                assert( oldNextNext->prev == oldNext );
            }
            if( oldNextNext != oldNext->next )
            {
                printf("        %d %d\n",oldNext->meta,oldNextNext->meta );
                printList();
                assert( oldNextNext == oldNext->next );
            }
        } while( !__sync_bool_compare_and_swap( &_head->next, oldNext, oldNextNext ) );
        if( !__sync_bool_compare_and_swap( &oldNextNext->prev, oldNext, _head ))
        {
            printf("@@@@@@\n");
        }
        if( oldNext == &dummy )
        {
            __sync_add_and_fetch( &dummy.size, -1 );
        }
        __sync_add_and_fetch( &_head->size, -1 );
    } while( oldNext == &dummy );

    /*
    assert( oldNextNext == oldNext->next );
    if( oldNextNext != oldNext->next )
    {
        assert( oldNextNext == _head );

        oldNextNext = oldNext->next;
        _head->next = oldNextNext;
        printf("        %d %d\n",oldNext->meta,oldNextNext->meta );
    }
    */

    return (struct ywq*)oldNext;
}

const int tryCount = 65536;
static struct ywq head;
static struct ywq slot[16][65536];

void printList()
{
    struct ywq * iter;

    YWQ_FOREACH( iter, &head )
    {
        printf("[%8x %8x %8d %8x]\n", 
                (unsigned int)iter->prev, 
                (unsigned int)iter, iter->meta,
                (unsigned int)iter->next );
    }
    printf("\n");
}

void * pushRoutine( void * arg )
{
    int          num = (int)arg;
    struct ywq * iter;
    int          i;
    int          prev=0;

    for( i = 0 ; i < 65536 ; i ++ )
    {
        slot[num][i].meta = i + 100000;
        ywqPush( &head, &slot[num][i] );
    }
}
void * popRoutine( void * arg )
{
    int          num = (int)arg;
    struct ywq * iter;
    int          i;
    int          prev=0;

    prev = 100000;
    for( i = 0 ; i < 65536 ; i ++ )
    {
        while( !(iter = ywqPop( &head )) );
        if( iter->meta != prev )
        {
            printf(" m : %d   p :%d\n", iter->meta, prev );
            assert( iter->meta == prev );
        }
        prev ++;
    }

    return (void*)num;
}

#define THREAD_COUNT 16
void   ywqTest()
{
    pthread_t    pt[16];
    int          i;
    int          j;
    int          k;
    struct ywq * iter;

    printf("BasicTest:\n");

    ywqInit( &head );
    slot[0][0].meta = 10000;
    ywqPush( &head, &slot[0][0] );
    iter = ywqPop( &head );
    printf("[%d]\n",iter->meta );
    ywqPush( &head, &slot[0][0] );
    iter = ywqPop( &head );
    printf("[%d]\n",iter->meta );

    printf("PushTest:\n");

    if(0)
    for( i = 0 ; i < 65536 ; i ++ )
    {
        ywqInit( &head );

        for( j = 0 ; j < THREAD_COUNT ; j ++)
            assert( 0 == pthread_create( &pt[j], NULL/*attr*/, pushRoutine, (void*)j ) );
        for( j = 0 ; j < THREAD_COUNT ; j ++)
            pthread_join( pt[j],(void**)&k );

        k = 0;
        YWQ_FOREACH_PREV( iter, &head )
            k++;
        assert( k == 65536*THREAD_COUNT);
        printf("%d\n",i);
    }

    printf("PushPullTest:\n");

    for( i = 0 ; i < 65536 ; i ++ )
    {
        dummy.size = 0;
        ywqInit( &head );
        assert( 0 == pthread_create( &pt[0], NULL/*attr*/, pushRoutine, (void*)0 ) );
        assert( 0 == pthread_create( &pt[1], NULL/*attr*/, popRoutine, (void*)1 ) );
        pthread_join( pt[0],(void**)&k );
        pthread_join( pt[1],(void**)&k );
    }
}
