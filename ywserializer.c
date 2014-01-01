#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ywutil.h"
#include "ywserializer.h"

#define VAR_DECL( type,name) type name;
#define VAR_WRITE(type,name) do{                       \
    if( sizeof(type)!=write( fd,&name,sizeof(type) ) ) \
    {                                                  \
        printf("%s %d\n",#name,sizeof(type));          \
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
#define VAR_PRINT(type,name) do{                       \
        printVar( #name, sizeof(type), (char*)&name ); \
    } while( 0 );

int ywsTest()
{
    int fd;

#define VAR_LIST(_)     \
    _(int, varA)        \
    _(int, varB)        \
    _(int, varC)        \
    _(int, varD)        \
    {
        VAR_LIST( VAR_DECL );

        varA = 1;
        varB = 2;
        varC = 3;
        varD = 4;

        fd = open( "ser.test", O_CREAT | O_RDWR );
        if( !fd ) return 0;
        VAR_LIST( VAR_WRITE );
        close( fd );

        varA=varB=varC=varD=0;
        VAR_LIST( VAR_PRINT );

        fd = open( "ser.test", O_RDONLY );
        if( !fd ) return 0;
        VAR_LIST( VAR_READ );
        close( fd );

        VAR_LIST( VAR_PRINT );
    }

#define VAR_LIST(_)       \
    _(int, varA)          \
    _(int, varB[seq]) \
    _(int, varC)          \
    _(int, varD)          \
    {
        VAR_LIST( VAR_DECL );

        varA = 1;
        varB = 2;
        varC = 3;
        varD = 4;

        fd = open( "ser.test", O_CREAT | O_RDWR );
        if( !fd ) return 0;
        VAR_LIST( VAR_WRITE );
        close( fd );

        varA=varB=varC=varD=0;
        VAR_LIST( VAR_PRINT );

        fd = open( "ser.test", O_RDONLY );
        if( !fd ) return 0;
        VAR_LIST( VAR_READ );
        close( fd );

        VAR_LIST( VAR_PRINT );
    }


    return 1;
}
