#ifndef __YW_UTIL_H__
#define __YW_UTIL_H__

#include "ywcommon.h"

void ywuGlobalInit();
void dump_stack();
void printVar( char * name, uint32_t size, char * buf );
void printHex( uint32_t size, char * buf );

#endif
