#include <execinfo.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <malloc.h>
#include <ctype.h>

#include "ywutil.h"

#define BUF_SIZE 8192

//typedef void (*__sighandler_t) (int);
void sig_handler(int signo)
{
    printf("I Received SIGINT(%d)\n", SIGINT);

    dump_stack();
}

void ywuGlobalInit()
{
    signal(SIGBUS, sig_handler);
    signal(SIGSEGV, sig_handler);
//    signal(SIGINT, SIG_IGN);
}


void dump_stack()
{
    int j, nptrs;
    void *buffer[8192];
    char **strings;

    nptrs = backtrace(buffer, BUF_SIZE);
    printf("backtrace() returned %d addresses\n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        return;
    }

    for (j = 0; j < nptrs; j++)
        printf("%s\n", strings[j]);
    free(strings);
}

void printVar( const char * name, uint32_t size, char * buf )
{
    printf("%-16s:",name);
    switch( size )
    {
        case 1:
            printf("0x%02x %u\n",*(uint8_t*)buf,*(uint8_t*)buf);
            break;
        case 2:
            printf("0x%04x %u\n",*(uint16_t*)buf,*(uint16_t*)buf);
            break;
        case 4:
            printf("0x%08x %u\n",*(uint32_t*)buf,*(uint32_t*)buf);
            break;
        case 8:
            printf("0x%016llx %llu\n",*(uint64_t*)buf,*(uint64_t*)buf);
            break;
        default:
            printf("\n");
            printHex( size, buf );
            break;
    }
}
void printHex( uint32_t size, char * buf )
{
    int i, j;
    int oldI;
    int line_size = 16;

    for( i=0; i<size; )
    {
        printf("0x%08x | ", i);
        oldI = i;
        for( j=0; j<line_size ; ++j,++i )
        {
            if( i < size )
            {
                printf("%02X",(uint8_t)buf[i]);
            }
            else
            {
                printf("  ");
            }

            if( (j&3) == 3 ) printf(" ");
        }
        printf(" | ");
        i=oldI;
        for( j=0; j<line_size; ++j,++i )
        {
            if( i < size )
            {
                if( isprint( buf[i] ) )
                    printf("%c",buf[i]);
                else
                    printf(".");
            }
            else
            {
                printf(" ");
            }
        }
        printf(" |\n");
    }
}
