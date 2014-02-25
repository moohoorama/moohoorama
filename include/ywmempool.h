/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWMEMPOOL_H_
#define INCLUDE_YWMEMPOOL_H_

#include <ywcommon.h>

class ywMemPool {
 public:
  ywMemPool() {
      printf("%s\n", typeid(this).name());
  }
 private:
};

#endif  // INCLUDE_YWMEMPOOL_H_
