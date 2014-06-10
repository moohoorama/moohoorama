/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */
#include <ywlogstore.h>
#include <ywarchive.h>
#include <gtest/gtest.h>

void ywLogStore::init(const char * fn, int32_t _io) {
    uint32_t  flag = O_RDWR | O_CREAT | O_LARGEFILE;

    append_pos.set_null();
    write_pos.set_null();
    prepare_log_cnk_cnt = 0;

    if (io == DIRECT_IO) flag |= O_DIRECT;
    fd = open(fn, flag, S_IRWXU);
    if (fd == -1) {
        perror("file open error:");
        assert(fd != -1);
    }

    init_and_read_master();

    assert(create_flush_thread());
}

void ywLogStore::init_and_read_master() {
    ywPos pos;

    assert(cnk_mgr.alloc_chunk(MASTER_CNK) == MASTER_CNK_ID);

    pos = create_chunk(LOG_CNK);
    assert(pos.get() == 1 * CHUNK_SIZE);

    append_pos = pos;
    write_pos  = pos;

    ywar_print  print_ar;
    cnk_mgr.dump(&print_ar);

    write_master_chunk();
}

void ywLogStore::write_master_chunk() {
    Byte cnk[1024];

    ywar_bytestream   byte_ar(cnk);
    cnk_mgr.dump(&byte_ar);
}

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
        log->reserve_space();
        if (!log->flush()) {
            ywWaitEvent::u_sleep(100, get_pc());
        }
    }
    log->running = false;

    return NULL;
}

void ywLogStore::reserve_space() {
    if (prepare_log_cnk_cnt < MAX_PREPARE_LOG_CNK_CNT) {
        (void)create_chunk(LOG_CNK);
    }
}

bool ywLogStore::flush() {
    struct    iovec iov[FLUSH_COUNT];
    ywCnkID   free_cnk_id[FLUSH_COUNT];
    uint32_t  free_cnk_cnt = 0;
    ssize_t   estimated_size = 0;
    size_t    write_phy_offset;
    uint64_t  old_val = write_pos.get();
    ywCnkID   id     = get_cnk_id(write_pos);
    int32_t   offset = get_offset(write_pos);
    Byte     *last_ptr = NULL;
    uint32_t  cnt;
    uint32_t  i;

    ywCnkLoc  p_loc = cnk_mgr.get_cnk_info(id).p_loc;

    write_phy_offset = p_loc * CHUNK_SIZE + offset;

    cnt = 0;
    do {
        if (write_pos.get() + FLUSH_UNIT > append_pos.get()) {
            break;
        }

        id     = get_cnk_id(write_pos);
        offset = get_offset(write_pos);

        ywBCID    bcid   = buff_cache.ht_find(id);
        if (buff_cache.is_null(bcid)) {
            break;
        }
        if (appender_view_pos.min() <= id) {
            break;
        }

        assert(offset % FLUSH_UNIT == 0);
        assert(offset + FLUSH_UNIT <= CHUNK_SIZE);

        ywChunk  *cnk = buff_cache.get_body(bcid);
        Byte     *ptr = &cnk->body[offset];

        /* continuous? */
        if (last_ptr == ptr && offset != 0) {
            iov[cnt-1].iov_len += FLUSH_UNIT; /* attach */
            assert(iov[cnt-1].iov_len <= CHUNK_SIZE);
        } else {
            iov[cnt].iov_base = ptr;
            iov[cnt].iov_len  = FLUSH_UNIT;
            ++cnt;
        }

        last_ptr = ptr + FLUSH_UNIT;

        if (offset + FLUSH_UNIT == CHUNK_SIZE) { /* chunk completed */
            free_cnk_id[free_cnk_cnt++] = id;
        }

        (void)write_pos.go_forward<CHUNK_SIZE>(FLUSH_UNIT);
        estimated_size += FLUSH_UNIT;
    } while (cnt < FLUSH_COUNT);

    if (!cnt) return false;

    if (io != NO_IO) {
        ssize_t    ret = pwritev(fd, iov, cnt, write_phy_offset);
        if (ret != estimated_size) {
            write_pos.set(old_val);
            perror("flush error:");
            return false;
        }
    }

    for (i = 0; i < free_cnk_cnt; ++i) {
        assert(free_cnk_id[i] < get_cnk_id(write_pos));
        buff_cache.set_victim(free_cnk_id[i]);
        --prepare_log_cnk_cnt;
    }

    return true;
}

void ywLogStore::wait_flush() {
    if (io != NO_IO) {
        while (write_pos.get() + FLUSH_UNIT <= append_pos.get()) {
            ywWaitEvent::u_sleep(10, get_pc());
        }
    }
}

class logStoreConcTest {
 public:
    static const int64_t TOTAL_TEST_SIZE = 8*GB;
    static const int64_t TEST_SIZE  = 512;

    void calc(int32_t thread_count) {
        test_count = TOTAL_TEST_SIZE / thread_count / TEST_SIZE;
        printf("test_count : %d\n", test_count);
    }
    void run();

    ywLogStore *ls;
    int32_t     operation;
    int32_t     test_count;
};

void logStoreConcTest::run() {
    int64_t           i;
    Byte              buf[TEST_SIZE];
    ywBarray          test(TEST_SIZE, buf);

    for (i = 0; i < test_count; ++i) {
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
    int32_t           i;
    ywPerfChecker     perf;

    for (i = 0; i < thread_count; ++i) {
        tc[i].operation  = i;
        tc[i].ls         = &ls;
        tc[i].calc(thread_count);
        assert(tpool->add_task(ls_stress_task, &tc[i]));
    }

    perf.start(logStoreConcTest::TOTAL_TEST_SIZE/1024/1024);
    tpool->wait_to_idle();

    printf("append : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_append_pos(),
           ls.get_append_pos()/1024/1024);
    printf("writed : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_write_pos(),
           ls.get_write_pos()/1024/1024);
    ls.wait_flush();
    printf("flushd : %" PRId64 "(%" PRId64 "MB)\n",
           ls.get_write_pos(),
           ls.get_write_pos()/1024/1024);
    perf.finish("MPS");

    ywWaitEvent::print();
//    ls.sync();
}
