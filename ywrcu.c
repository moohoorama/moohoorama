#include <unistd.h>
#include <pthread.h>
#include <unistd.h>

#include "ywrcu.h"

/************ DECALRE *************/

/*
void * _rcuTimer( void * arg );
void   _rcuSetState( struct rcu_ref * _ref, uint32_t nextState);

static int           rcu_time;
static struct ywq    freeQueue;

void * _rcuTimer( void * arg )
{
    while( 1 )
    {
        usleep( 100 );
        if( rcu_time < RCU_TIME_MAX )
        {
            rcu_time ++;
        }
        else
        {
            rcu_time = 0;
        }
    }
}

void   _rcuSetState( struct rcu_ref * _ref, uint32_t nextState)
{
    int prevState;
    do
    {
        prevState = _ref->state;
        if( prevState == RCU_STATE_LOCKED )
        {
            usleep( RCU_SLEEP_TIME );
            continue;
        }
    }
   while( !__sync_bool_compare_and_swap( 
               &_ref->state, prevState, nextState ) );
}

void rcuGlobalInit()
{
    int         ret = 0;
    pthread_t   hthread;

    ywqInit( &freeQueue );

    rcu_time = 0;
    ret = pthread_create( &hthread, NULL//attr, 
        _rcuTimer, NULL//arg
        );
    assert( ret == 0 );
}

void    rcuInit( struct rcu_ref * _ref )
{
    _ref->state = RCU_STATE_STABLE;
    _ref->ref   = NULL;
}
void rcuSet( struct rcu_ref * _ref, void * target )
{
    void * prevRef = _ref->ref;
    _rcuSetState( _ref, RCU_STATE_LOCKED );
    prevRef     = _ref->ref;
    _ref->ref   = target;
    _ref->state = RCU_STATE_STABLE;

    if( prevRef != NULL )
    {
        ywqPush( &freeQueue, prevRef, rcu_time );
//        freeList.push( prevRef, rcu_time );
    }
}

void rcuGlobalDest()
{

}
*/
