/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywtimer.h>
#include <gtest/gtest.h>

ywTimer ywTimer::gInstance;

void *ywTimer::run(void *arg) {
    ywTimer *timer = ywTimer::get_instance();
    timer->global_timer();

    return NULL;
}

void ywTimer::global_timer() {
    int32_t i;
    int32_t idx = 0;

    while (true) {
        for (i = 0; i < count; ++i) {
            if ((idx % interval[i]) == 0) {
                func[i](arg[i]);
            }
        }
        ++idx;
//        usleep(MIN_INTERVAL_USEC);
    }
}

