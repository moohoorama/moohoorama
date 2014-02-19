/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWHAZARDPOINTER_H_
#define INCLUDE_YWHAZARDPOINTER_H_

#include <ywdlist.h>
#include <ywspinlock.h>

class ywHazzardPointer {
    bool assignHazardPointer(void * ptr);
    bool getLink(void ** ptr);

    ywSpinLock  listLock;
    ywList      pointerList;
};

#endif  // INCLUDE_YWHAZARDPOINTER_H_

