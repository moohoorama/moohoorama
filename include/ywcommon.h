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
#include <string.h>
#include <typeinfo>
#include <type_traits>

static const size_t  KB = 1024;
static const size_t  MB = 1024*KB;
static const size_t  GB = 1024*MB;

typedef uint8_t Byte;

template<typename PARENT, typename CHILD>
inline void check_inheritance() {
    static_assert(std::is_base_of<PARENT, CHILD>::value, "invalid class");
}

#endif  // INCLUDE_YWCOMMON_H_
