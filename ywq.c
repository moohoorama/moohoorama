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
struct ywq  * ywqPop( struct ywq * _head )
{
    struct ywq * target = NULL;
    struct ywq * linker = NULL;

    if( _head->prev == _head )
    {
        return NULL; /* Really Emtpy */
    }

    do
    {
        target = _head->next;
        linker = target->next;
        while( target->prev != _head )
        {
            linker = target;
            target = target->prev;
        }
    } while( !__sync_bool_compare_and_swap( &linker->prev, target, _head ) );
    
    _head->next = linker;

    return target;
}

#define tryCount 1024
static struct ywq head;
static struct ywq slot[16][tryCount];

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
    int          i;

    for( i = 0 ; i < tryCount ; i ++ )
    {
        slot[num][i].meta = i + 100000;
        ywqPush( &head, &slot[num][i] );
    }

    return NULL;
}
void * popRoutine( void * arg )
{
    int          num = (int)arg;
    struct ywq * iter;
    int          i;
    int          prev=0;

    prev = 100000;
    for( i = 0 ; i < tryCount ; i ++ )
    {
        while( !(iter = ywqPop( &head )) );
        if( iter->meta != prev )
        {
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

    for( i = 0 ; i < tryCount ; i ++ )
    {
        ywqInit( &head );

        for( j = 0 ; j < THREAD_COUNT ; j ++)
            assert( 0 == pthread_create( &pt[j], NULL/*attr*/, pushRoutine, (void*)j ) );
        for( j = 0 ; j < THREAD_COUNT ; j ++)
            pthread_join( pt[j],(void**)&k );

        k = 0;
        YWQ_FOREACH_PREV( iter, &head )
            k++;
        assert( k == tryCount*THREAD_COUNT);
    }

    printf("PushPullTest:\n");

    ywqInit( &head );
    for( i = 0 ; i < tryCount ; i ++ )
    {
        assert( 0 == pthread_create( &pt[0], NULL/*attr*/, pushRoutine, (void*)0 ) );
        assert( 0 == pthread_create( &pt[1], NULL/*attr*/, popRoutine, (void*)1 ) );
        pthread_join( pt[0],(void**)&k );
        pthread_join( pt[1],(void**)&k );
    }
}
