#include <ywspinlock.h>

bool ywSpinLock::tryRLock(){
    uint32_t prev = status;
    if (prev < 0)
        return false;
    return __sync_bool_compare_and_swap(&status, prev, prev+1);
}

bool ywSpinLock::tryWLock(){
    uint32_t prev = status;
    if (prev)
        return false;
    return __sync_bool_compare_and_swap(&status, prev, WLOCK);
}

bool ywSpinLock::RLock(uint32_t timeout){
    int i;

    for (i = 0; i < timeout; ++i) {
        if (tryRLock()){
            return true;
        }
    }
    miss_count++;

    return false;
}

bool ywSpinLock::WLock(uint32_t timeout){
    int i;

    for (i = 0; i < timeout; ++i) {
        if (tryWLock()){
            return true;
        }
    }
    miss_count++;

    return false;
}
