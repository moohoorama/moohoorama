/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywtimer.h>
#include <gtest/gtest.h>

ywTimer ywTimer::gInstance;

ywAccumulator<intptr_t>     ywWaitEvent::wait_event;
ywAccumulator<intptr_t>     ywWaitEvent::last_sleep_loc;
ywAccumulator<intptr_t>     ywWaitEvent::acc_sleep_time;
std::map<intptr_t, ssize_t> ywWaitEvent::wait_sum;

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
        ywWaitEvent::u_sleep(MIN_INTERVAL_USEC, get_pc());
    }
}

