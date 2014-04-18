/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#include <ywlogstore.h>
#include <gtest/gtest.h>

bool ywLogStore::create_flush_thread() {
    pthread_attr_t          attr;
    pthread_t               pt;
    assert(0 == pthread_attr_init(&attr) );

    assert(0 == pthread_create(&pt, &attr, log_flusher, this));
    return true;
}

void *ywLogStore::log_flusher(void *arg_ptr) {
    ywLogStore *log = reinterpret_cast<ywLogStore*>(arg_ptr);

    log->running = true;
    while (!log->done) {
        if (!log->flush()) {
            usleep(100);
        }
    }
    log->running = false;

    return NULL;
}

class logStoreConcTest {
 public:
    static const int32_t TEST_COUNT = 16*KB;

    void run();

    ywLogStore *ls;
    int32_t     operation;
};

void logStoreConcTest::run() {
    int32_t           i;
    Byte              buf[64*KB];

    for (i = 0; i < TEST_COUNT; ++i) {
        ywBarray   test(64*KB, buf);
//        ywLong test(i);
        ls->append(&test);
    }
}

void ls_stress_task(void *arg) {
//    logStoreConcTest *tc = reinterpret_cast<logStoreConcTest*>(arg);
    auto *tc = reinterpret_cast<logStoreConcTest*>(arg);

    tc->run();
}


void logstore_basic_test(int32_t io_type, int32_t thread_count) {
    ywWorkerPool     *tpool = ywWorkerPool::get_instance();
    logStoreConcTest  tc[MAX_THREAD_COUNT];
    ywLogStore        ls("test.txt", io_type);
//    Byte              buf[64*KB];
//    size_t            i;
    int32_t           i;
    ywPerfChecker     perf;

    for (i = 0; i < thread_count; ++i) {
        tc[i].operation = i;
        tc[i].ls        = &ls;
        assert(tpool->add_task(ls_stress_task, &tc[i]));
    }
    perf.start(logStoreConcTest::TEST_COUNT*thread_count*64/1024);
    tpool->wait_to_idle();

    printf("append : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_append_pos(),
           ls.get_append_pos()/1024/1024);
    printf("flushd : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_write_pos(),
           ls.get_write_pos()/1024/1024);
    ls.wait_flush();
    printf("flushd : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_write_pos(),
           ls.get_write_pos()/1024/1024);
    perf.finish("MPS");
//    ls.sync();
}
