#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include "ywdlist.h"
#include "ywq.h"
#include "ywutil.h"
#include "ywserializer.h"
#include "ywmain.h"

#define cpuid(func,ax,bx,cx,dx)        \
        __asm__ __volatile__ ("cpuid": \
       "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));

void * route2( void * arg )
{
    int          a,b,c,d;

    cpuid(0,a,b,c,d);
    printf("%x %x %x %x\n", a,b,c,d);

    return NULL;
}

int main( int argc, char ** argv )
{
    pthread_t    pt[64];
    int          i;
    int          j;

    ywGlobalInit();

    ywqTest();
    assert( ywsTest() );

    printHex( 128, (char*)&pt );

    for( i=0; i<8; i++ )
    {
        assert( 0 == 
                pthread_create( 
                    &pt[i], 
                    NULL/*attr*/, 
                    route2, 
                    (void*)1 ) );
    }
    for( i=0; i<8; i++ )
    {
        pthread_join( pt[i],(void**)&j );
    }

    dump_stack();

    return 0;
}
