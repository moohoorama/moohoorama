#ifndef __YW_SERIALIZER_H__
#define __YW_SERIALIZER_H__

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

/* precondition : declare variables fd & size */
#define VAR_WRITE(type,name) do{                       \
    if( sizeof(type)!=write( fd,&name,sizeof(type) ) ) \
    {                                                  \
        printVar( #name, sizeof(type), (char*)&name ); \
        return 0;                                      \
    }                                                  \
    } while( 0 );
#define VAR_READ(type,name) do{                        \
    if( sizeof(type)!=read( fd,&name,sizeof(type) ) )  \
    {                                                  \
        printVar( #name, sizeof(type), (char*)&name ); \
        return 0;                                      \
    }                                                  \
    } while( 0 );
#define VAR_PRINT(type,name)  printVar( #name, sizeof(type), (char*)&name );
#define VAR_SIZE(type,name)  size += sizeof(type);

/* example */
int ywsTest();

#endif  /*__YW_SERIALIZER_H__ */
