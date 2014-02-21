/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWCOMMON_H_
#define INCLUDE_YWCOMMON_H_

#define _LIKELY(cond) __builtin_expect(cond, true)
#define _UNLIKELY(cond) __builtin_expect(cond, false)

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#endif  // INCLUDE_YWCOMMON_H_
