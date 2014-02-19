/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YW_PAGE_MANAGER_H_
#define INCLUDE_YW_PAGE_MANAGER_H_

#include <ywdlist.h>
#include <ywspinlock.h>

class ywPageManager {
    bool assign_PAGE_MANAGER(void * ptr);
    bool getLink(void ** ptr);

    ywSpinLock  listLock;
    ywList      pointerList;
};

#endif  // INCLUDE_YW_PAGE_MANAGER_H_

