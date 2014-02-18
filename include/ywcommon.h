/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWCOMMON_H_
#define INCLUDE_YWCOMMON_H_

#define _LIKELY(cond) __builtin_expect(cond, true)
#define _UNLIKELY(cond) __builtin_expect(cond, false)

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#endif  // INCLUDE_YWCOMMON_H_
