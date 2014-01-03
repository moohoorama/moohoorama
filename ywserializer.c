#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ywutil.h"
#include "ywserializer.h"

int ywsTest()
{
    int fd;

#define VAR_LIST(OPR)     \
    OPR(int, varA)        \
    OPR(int, varB)        \
    OPR(int, varC)        \
    OPR(int, varD)
    {
        int varA;
        int varB;
        int varC;
        int varD;

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
#undef  VAR_LIST

#define VAR_LIST(OPR)                 \
    OPR(int, varA)                    \
    for(int seq=0; seq<varA; seq++){  \
        OPR(int, varB[seq])           \
    }                                 \
    OPR(int, varC)                    \
    OPR(int, varD)
    {
        int varA;
        int varB[16]={5,6};
        int varC;
        int varD;

        varA = 3;
        varC = 3;
        varD = 4;

        fd = open( "ser.test", O_CREAT | O_RDWR );
        if( !fd ) return 0;
        VAR_LIST( VAR_WRITE );
        close( fd );

        VAR_LIST( VAR_PRINT );

        fd = open( "ser.test", O_RDONLY );
        if( !fd ) return 0;
        VAR_LIST( VAR_READ );
        close( fd );

        VAR_LIST( VAR_PRINT );
    }
#undef  VAR_LIST


    return 1;
}
