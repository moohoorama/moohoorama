#ifndef __YW_RCU_H__
#define __YW_RCU_H__

#include <stdint.h>

#define RCU_TIME_MAX      ( UINT32_MAX-2 )
#define RCU_STATE_STABLE  ( RCU_TIME_MAX+1 )
#define RCU_STATE_LOCKED  ( RCU_TIME_MAX+2 )
#define RCU_SLEEP_TIME    ( 100 )


struct rcu_ref
{
    uint32_t   state;
    void     * ref;
};

void rcuGlobalInit();
void rcuGlobalDest();

void    rcuInit( struct rcu_ref * _ref );
void    rcuSet( struct rcu_ref * _ref, void * target );
void  * rcuGet( struct rcu_ref * _ref );
void  * rcuGetFree( struct rcu_ref * _ref );

inline void  * rcuGet( struct rcu_ref * _ref )
{
    return _ref->ref;
}

#endif /* __YW_RCU_H__ */
