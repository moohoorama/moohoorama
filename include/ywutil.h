/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWUTIL_H_
#define INCLUDE_YWUTIL_H_

#include <ywcommon.h>

void ywuGlobalInit();
void dump_stack();
void printVar(const char * name, uint32_t size, char * buf);
void printHex(uint32_t size, char * buf);

#endif  // INCLUDE_YWUTIL_H_
